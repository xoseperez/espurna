import { notifyError, notifyMessage } from './notify.mjs';
import { pageReloadIn } from './core.mjs';

/** @typedef {{auth: URL, config: URL, upgrade: URL, ws: URL}} ConnectionUrls */

/**
 * @param {URL} root
 * @returns {URL}
 */
function makeWebSocketUrl(root) {
    const out = new URL("ws", root);
    out.protocol =
        (root.protocol === "https:")
            ? "wss:"
            : "ws:";

    return out;
}

/**
 * @param {string} path
 * @param {URL} root
 * @returns {URL}
 */
function makeUrl(path, root) {
    let out = new URL(path, root);
    out.protocol = root.protocol;
    return out;
}

/**
 * @param {URL} root
 * @returns ConnectionUrls
 */
function makeConnectionUrls(root) {
    return {
        auth: makeUrl("auth", root),
        config: makeUrl("config", root),
        upgrade: makeUrl("upgrade", root),
        ws: makeWebSocketUrl(root),
    };
}
class ConnectionBase {
    constructor() {
        /** @type {WebSocket | null} */
        this._socket = null;

        /** @type {NodeJS.Timeout | number | null} */
        this._ping_pong = null;

        /** @type {ConnectionUrls | null} */
        this._urls = null;
    }

    /** @returns {boolean} */
    connected() {
        return this._ping_pong !== null;
    }

    /** @returns {ConnectionUrls | null} */
    urls() {
        return this._urls !== null
            ? Object.assign({}, this._urls)
            : null;
    }
};

/**
 * @callback OnMessage
 * @param {MessageEvent} event
 * @param {ConnectionBase} connection
 * @returns {any}
 */

/**
 * @callback OnOpen
 * @param {Event} event
 * @param {ConnectionBase} connection
 * @returns {any}
 */

/**
 * @callback OnClose
 * @param {CloseEvent} event
 * @param {ConnectionBase} connection
 * @returns {any}
 */

/**
 * @typedef {{onmessage?: OnMessage | null, onopen?: OnOpen | null, onclose?: OnClose | null}} ConnectionOptions
 */

/**
 * @param {ConnectionUrls} urls
 * @param {ConnectionOptions} options
 */
ConnectionBase.prototype.open = function(urls, {onopen = null, onclose = null, onmessage = null} = {}) {
    this._socket = new WebSocket(urls.ws.href);
    this._socket.onopen = (event) => {
        const id = setInterval(
            () => { sendAction("ping"); }, 5000);
        this._ping_pong = id;
        if (onopen) {
            onopen(event, this);
        }
    };
    this._socket.onclose = (event) => {
        if (this._ping_pong) {
            clearInterval(this._ping_pong);
            this._ping_pong = null;
        }
        if (onclose) {
            return onclose(event, this);
        }
    };

    if (onmessage) {
        this._socket.onmessage = (event) => {
            return onmessage(event, this);
        };
    }
}

/**
 * @param {string} payload
 * @throws {Error}
 */
ConnectionBase.prototype.send = function(payload) {
    if (!this._socket) {
        throw new Error("WebSocket disconnected!");
    }

    this._socket.send(payload);
}

const Connection = new ConnectionBase();

/**
 * @returns {boolean}
 */
export function isConnected() {
    return Connection.connected();
}

/**
 * @param {ConnectionUrls} urls
 * @param {ConnectionOptions} options
 */
function onAuthorized(urls, options) {
    Connection.open(urls, options);
    window.dispatchEvent(
        new CustomEvent("app-connected", {detail: {urls}}));
}

/**
 * @param {Error} error
 */
function onFetchError(error) {
    notifyError(error);
    pageReloadIn(5000);
}

/**
 * @param {Response} response
 */
function onError(response) {
    notifyMessage(`${response.url} responded with status code ${response.status}, reloading the page`);
    pageReloadIn(5000);
}

/**
 * @param {URL} root
 * @param {ConnectionOptions} options
 */
async function connectToURL(root, options) {
    const urls = makeConnectionUrls(root);

    /** @type {RequestInit} */
    const opts = {
        'method': 'GET',
        'credentials': 'same-origin',
        'mode': 'cors',
    };

    try {
        const response = await fetch(urls.auth.href, opts);
        // Set up socket connection handlers
        if (response.status === 200) {
            onAuthorized(urls, options);
        // Nothing to do, reload page and retry on errors
        } else {
            onError(response);
        }
    } catch (e) {
        onFetchError(/** @type {Error} */(e));
    }
}

/** @param {Event} event */
async function onConnectEvent(event) {
    const detail = /** @type {CustomEvent<{url: URL, options: ConnectionOptions}>} */
        (event).detail;
    await connectToURL(
        detail.url, detail.options);
}

/** @param {Event} event */
function onSendEvent(event) {
    Connection.send(/** @type {CustomEvent<{data: string}>} */
        (event).detail.data);
}

/** @param {function(ConnectionUrls): void} callback */
export function listenAppConnected(callback) {
    window.addEventListener("app-connected", (event) => {
        event.preventDefault();

        const urls = /** @type {CustomEvent<{urls: ConnectionUrls}>} */
            (event).detail.urls;
        callback(urls);
    });
}

/** @param {string} data */
export function send(data) {
    window.dispatchEvent(
        new CustomEvent("app-send", {detail: {data}}));
}

/**
 * @param {string} action
 * @param {any?} data
 */
export function sendAction(action, data = {}) {
    send(JSON.stringify({action, data}));
}

/** @param {ConnectionOptions} options */
export function connect(options) {
    // Optionally, support host=... param that could redirect to somewhere else
    // Note of the Cross-Origin rules that apply, and require device to handle them
    const search = new URLSearchParams(window.location.search);

    let host = search.get("host");
    if (host && !host.startsWith("http:") && !host.startsWith("https:")) {
        host = `http://${host}`;
    }

    const url = (host) ? new URL(host) : window.location;
    window.dispatchEvent(
        new CustomEvent("app-connect", {detail: {url, options}}));
}

export function init() {
    window.addEventListener("app-connect", onConnectEvent);
    window.addEventListener("app-send", onSendEvent);
}
