//
// Copyright 2025-2026, Phillip Heller
//
// This file is part of Prodigy Reloaded.
//
// Prodigy Reloaded is free software: you can redistribute it and/or modify it under the terms of the GNU General
// Public License as published by the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// Prodigy Reloaded is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with Prodigy Reloaded. If not,
// see <https://www.gnu.org/licenses/>.
//
import * as path from 'path';
import * as fs from 'fs';
import {
    workspace,
    ExtensionContext,
    Selection,
    Range,
    Uri,
    window,
    commands,
    tasks,
    Task,
    TaskDefinition,
    TaskGroup,
    TaskScope,
    ShellExecution,
    TaskProvider,
    languages,
    Color,
    ColorInformation,
    ColorPresentation
} from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

/* -- Public API for sibling extensions -------------------------------- */

/**
 * API surface exported via getExtensionApi() for sibling Prodigy extensions
 * (e.g., a page compositor that needs to query TBOL symbol information).
 */
export interface TbolExtensionApi {
    /** The running language client, or undefined if activation failed. */
    readonly languageClient: LanguageClient | undefined;
}

/* -- Path utilities --------------------------------------------------- */

/**
 * Expand VS Code variables like ${workspaceFolder} in a path
 */
function expandPath(p: string): string {
    const workspaceFolder = workspace.workspaceFolders?.[0]?.uri.fsPath || '';
    return p
        .replace(/\$\{workspaceFolder\}/g, workspaceFolder)
        .replace(/\$\{workspaceRoot\}/g, workspaceFolder);  // Legacy alias
}

/**
 * Get include paths with variables expanded
 */
function getIncludePaths(): string[] {
    const rawPaths = workspace.getConfiguration('tbol').get<string[]>('includePaths') || [];
    return rawPaths.map(expandPath);
}

/**
 * Search for a tool executable using a standard fallback chain:
 * 1. User-configured path (settings)
 * 2. Bundled in extension's bin/ directory
 * 3. Sibling directory (development mode)
 * 4. System PATH
 */
function findExecutable(
    context: ExtensionContext,
    name: string,
    configKey: string,
    devSubdir: string
): string | null {
    const configPath = workspace.getConfiguration('tbol').get<string>(configKey);
    if (configPath && fs.existsSync(configPath)) {
        return configPath;
    }

    const bundledPath = path.join(context.extensionPath, 'bin', name);
    if (fs.existsSync(bundledPath)) {
        return bundledPath;
    }

    const devPath = path.join(context.extensionPath, '..', devSubdir, name);
    if (fs.existsSync(devPath)) {
        return devPath;
    }

    const pathDirs = (process.env.PATH || '').split(path.delimiter);
    for (const dir of pathDirs) {
        const candidate = path.join(dir, name);
        if (fs.existsSync(candidate)) {
            return candidate;
        }
    }

    return null;
}

/* -- Extension lifecycle ---------------------------------------------- */

export function activate(context: ExtensionContext): TbolExtensionApi {
    const serverPath = findExecutable(context, 'tbol-lsp', 'serverPath', 'lsp');

    if (!serverPath) {
        window.showErrorMessage(
            'TBOL language server not found. Please set tbol.serverPath in settings or rebuild the extension.'
        );
        return { languageClient: undefined };
    }

    console.log('TBOL: Using language server at', serverPath);
    console.log('TBOL: Include paths:', getIncludePaths());

    // Server options - run the language server as a stdio process
    const serverOptions: ServerOptions = {
        run:   { command: serverPath, transport: TransportKind.stdio },
        debug: { command: serverPath, transport: TransportKind.stdio }
    };

    // Client options
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'tbol' }],

        synchronize: {
            configurationSection: 'tbol'
        },

        // Expand ${workspaceFolder} in includePaths before sending to server
        middleware: {
            workspace: {
                configuration: async (params, token, next) => {
                    const result = await next(params, token);
                    if (Array.isArray(result)) {
                        for (const item of result) {
                            if (item && Array.isArray(item.includePaths)) {
                                item.includePaths = item.includePaths.map(expandPath);
                            }
                        }
                    }
                    return result;
                }
            }
        }
    };

    client = new LanguageClient('tbol', 'TBOL Language Server', serverOptions, clientOptions);
    client.start();

    // Register task provider for tbolc compilation
    context.subscriptions.push(
        tasks.registerTaskProvider('tbol', new TbolTaskProvider(context))
    );

    // Register bytecode-browsing commands
    context.subscriptions.push(
        commands.registerCommand('tbol.decompile', () => runDecompiler(context))
    );
    context.subscriptions.push(
        commands.registerCommand('tbol.disassemble', () => runDisassembler(context))
    );

    // Register URI handler for cross-extension navigation.
    // Sibling extensions (e.g., a page compositor) can open TBOL source files
    // via URIs like vscode://prodigy-tbol/open?file=/path/to/file.SRC&line=10&column=5
    context.subscriptions.push(
        window.registerUriHandler({
            async handleUri(uri: Uri) {
                const query = new URLSearchParams(uri.query);
                const filePath = query.get('file');
                if (!filePath) { return; }

                const line = parseInt(query.get('line') || '1', 10);
                const column = parseInt(query.get('column') || '1', 10);

                const fileUri = Uri.file(filePath);
                const editor = await window.showTextDocument(fileUri);
                const pos = editor.document.positionAt(0).with(
                    Math.max(0, line - 1),
                    Math.max(0, column - 1)
                );
                editor.selection = new Selection(pos, pos);
                editor.revealRange(new Range(pos, pos));
            }
        })
    );

    // Suppress VS Code's built-in color picker for #nnnn GEV references
    context.subscriptions.push(
        languages.registerColorProvider({ language: 'tbol' }, {
            provideDocumentColors(): ColorInformation[] { return []; },
            provideColorPresentations(): ColorPresentation[] { return []; }
        })
    );

    console.log('TBOL language extension activated');
    return { languageClient: client };
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}

/* -- Bytecode browsing commands --------------------------------------- */

/**
 * Run tboldc on a .COD or .PGM file and open the recovered TBOL source in
 * a new editor.  The -f flag forces output even if round-trip verification
 * fails, so the user always sees what was recovered; any verification
 * warnings from stderr surface as a VS Code warning so the user knows the
 * output may not be byte-faithful.
 */
async function runDecompiler(context: ExtensionContext): Promise<void> {
    const dcPath = findExecutable(context, 'tboldc', 'decompilerPath', 'decompiler');
    if (!dcPath) {
        window.showErrorMessage(
            'tboldc not found. Please set tbol.decompilerPath in settings or ensure tboldc is in PATH.'
        );
        return;
    }

    const fileUris = await window.showOpenDialog({
        canSelectMany: false,
        filters: { 'TBOL bytecode': ['cod', 'COD', 'pgm', 'PGM'] },
        openLabel: 'Decompile'
    });

    if (!fileUris || fileUris.length === 0) {
        return;
    }

    const inputFile = fileUris[0].fsPath;
    const includePaths = getIncludePaths();
    const includeArgs = includePaths.flatMap(p => ['-I', p]);

    const { execFile } = await import('child_process');
    const { promisify } = await import('util');
    const execFileAsync = promisify(execFile);

    try {
        const { stdout, stderr } = await execFileAsync(dcPath, [...includeArgs, '-f', inputFile]);
        if (stderr && stderr.trim()) {
            window.showWarningMessage(`tboldc: ${stderr.trim()}`);
        }
        const doc = await workspace.openTextDocument({ content: stdout, language: 'tbol' });
        await window.showTextDocument(doc);
    } catch (err: unknown) {
        const msg = err instanceof Error ? err.message : String(err);
        window.showErrorMessage(`Decompilation failed: ${msg}`);
    }
}

/**
 * Run tboldasm on a .COD or .PGM file and open the raw instruction listing
 * in a new editor.  The disassembler does not recover expressions or
 * control flow and its output is not re-compilable.  For everyday browsing
 * of a bytecode file, use runDecompiler() instead; this command exists for
 * low-level analysis and compiler-development work.
 */
async function runDisassembler(context: ExtensionContext): Promise<void> {
    const dasmPath = findExecutable(context, 'tboldasm', 'disassemblerPath', 'disassembler');
    if (!dasmPath) {
        window.showErrorMessage(
            'tboldasm not found. Please set tbol.disassemblerPath in settings or ensure tboldasm is in PATH.'
        );
        return;
    }

    const fileUris = await window.showOpenDialog({
        canSelectMany: false,
        filters: { 'TBOL bytecode': ['cod', 'COD', 'pgm', 'PGM'] },
        openLabel: 'Disassemble'
    });

    if (!fileUris || fileUris.length === 0) {
        return;
    }

    const inputFile = fileUris[0].fsPath;
    const includePaths = getIncludePaths();
    const includeArgs = includePaths.flatMap(p => ['-I', p]);

    const { execFile } = await import('child_process');
    const { promisify } = await import('util');
    const execFileAsync = promisify(execFile);

    try {
        const { stdout, stderr } = await execFileAsync(dasmPath, [...includeArgs, inputFile]);
        if (stderr) {
            window.showWarningMessage(`tboldasm: ${stderr.trim()}`);
        }
        const doc = await workspace.openTextDocument({ content: stdout, language: 'tbol' });
        await window.showTextDocument(doc);
    } catch (err: unknown) {
        const msg = err instanceof Error ? err.message : String(err);
        window.showErrorMessage(`Disassembly failed: ${msg}`);
    }
}

/* -- Build task provider ---------------------------------------------- */

interface TbolTaskDefinition extends TaskDefinition {
    file: string;
}

/**
 * Task provider for TBOL compilation.
 * Provides a default "tbolc: compile" build task that compiles the current file.
 */
class TbolTaskProvider implements TaskProvider {
    constructor(private context: ExtensionContext) {}

    provideTasks(): Task[] {
        const compilerPath = findExecutable(this.context, 'tbolc', 'compilerPath', 'compiler');
        if (!compilerPath) {
            return [];
        }

        const includePaths = getIncludePaths();
        const includeArgs = includePaths.flatMap(p => ['-I', p]);

        const definition: TbolTaskDefinition = { type: 'tbol', file: '${file}' };
        const task = new Task(
            definition,
            TaskScope.Workspace,
            'compile',
            'tbolc',
            new ShellExecution(compilerPath, [...includeArgs, '${file}']),
            '$tbolc'
        );
        task.group = TaskGroup.Build;
        return [task];
    }

    resolveTask(task: Task): Task | undefined {
        const definition = task.definition as TbolTaskDefinition;
        if (definition.file) {
            const compilerPath = findExecutable(this.context, 'tbolc', 'compilerPath', 'compiler');
            if (!compilerPath) {
                return undefined;
            }
            const includePaths = getIncludePaths();
            const includeArgs = includePaths.flatMap(p => ['-I', p]);
            task.execution = new ShellExecution(compilerPath, [...includeArgs, definition.file]);
            return task;
        }
        return undefined;
    }
}
