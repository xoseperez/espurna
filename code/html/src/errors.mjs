/**
 * @param {Error} error
 * @returns {string}
 */
export function formatError(error) {
    if (error.stack) {
        return error.stack;
    }

    return [
        error.name,
        error.message,
        "(no error stack)"
    ].join("\n");
}

/**
 * @param {string} source
 * @param {number} lineno
 * @param {number} colno
 */
export function formatSource(source, lineno, colno) {
    return `${source || "?"}:${lineno ?? "?"}:${colno ?? "?"}:`;
}

/**
 * @param {string} message
 * @param {string} source
 * @param {number} lineno
 * @param {number} colno
 * @param {any} error
 * @return {string}
 */
export function formatErrorEvent(message, source, lineno, colno, error) {
    let text = "";
    if (message) {
        text += message;
    }

    if (source || lineno || colno) {
        text += ` ${source || "?"}:${lineno ?? "?"}:${colno ?? "?"}:`;
    }

    if (error instanceof Error) {
        text += formatError(error);
    }

    return text;
}
