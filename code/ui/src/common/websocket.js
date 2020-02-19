// #!if ENV === 'development'
import mockServer from "./mockWebsocket";
import {alertError} from "./notification";
// #!endif

let Ws = function () {
};
Ws.prototype = {
    ws: null,
    pingInterval: null,
    _onMsg: null,
    args: null,
    retryTimeout: 1000,
    queue: {},
    urls: null,
    id: 0,
    retry(time, tries) {
        if (this.retryTimeout) {
            return;
        }

        this.retryTimeout = setTimeout(() => {
            this.retryTimeout = null;
            this.connectToUrl.apply(this, this.args);
        }, time);

        if (tries > 5) {
            if (confirm("Connection lost with the device, click OK to refresh the page")) {
                window.location.reload();
            }
        }
    },
    close() {
        if (this.ws && this.ws.readyState !== WebSocket.CLOSING && this.ws.readyState !== WebSocket.CLOSED) {
            this.ws.onclose = null;
            clearInterval(this.pingInterval);
            this.ws.close(...arguments);
        }
    },
    connect(host, cb) {
        // #!if ENV === 'development'
        if (!host || host.match("127.0.0.1")) {
            //Start mocking
            this.ws = mockServer();
            this._onMsg = cb;
            this.ws.onmessage = this.onMessage.bind(this);
            this.ws.onclose = this.onClose.bind(this);
            this.ws.onopen = this.onOpen.bind(this);
            return;
        }
        // #!endif

        if (!host.startsWith("http:") && !host.startsWith("https:")) {
            host = "http://" + host;
        }
        this.connectToUrl(new URL(host), cb);
    },
    connectToUrl(url, onMsg, tries) {
        tries = tries || 1;

        this.args = Array.prototype.slice.call(arguments);

        if (tries <= 1) {
            this.initUrls(url);
        }

        fetch(this.urls.auth.href, {
            "method": "GET",
            "cors": true,
            "credentials": "same-origin"
        }).then((response) => {
            // Failed, retry
            if (response.status !== 200) {
                return this.retry(5000, tries + 1);
            }
            this.close();
            this._onMsg = onMsg;
            // update websock object
            this.ws = new WebSocket(this.urls.ws.href);
            this.ws.onmessage = this.onMessage.bind(this);
            this.ws.onclose = this.onClose.bind(this);
            this.ws.onopen = this.onOpen.bind(this);

        }).catch((error) => {
            alertError({title: "Unable to connect to websocket, retrying in 5 seconds", message: error});
            return this.retry(5000, tries + 1);
        });
    },
    initUrls(root) {
        const urls = {};
        const paths = ["ws", "upgrade", "config", "auth"];

        urls["root"] = root;
        paths.forEach((path) => {
            urls[path] = new URL(path, root);
            urls[path].protocol = root.protocol;
        });

        if (root.protocol === "https:") {
            urls.ws.protocol = "wss:";
        } else {
            urls.ws.protocol = "ws:";
        }
        this.urls = urls;
    },
    send(payload, cb, repl) {
        if (this.ws) {
            if (cb) {
                payload.id = ++this.id;
                this.queue[payload.id] = cb;
            }
        }
        this.ws.send(
            JSON.stringify(payload,
                repl ? ((key, value) => typeof value === "undefined" ? null : value) : undefined)
        );
    },
    onClose() {
        this.retry(1000);
        clearInterval(this.pingInterval);
    },
    onOpen() {
        this.pingInterval = setInterval(() => {
            this.send({action: "ping"});
        }, 30000);
    },
    onMessage(evt) {
        let data = evt.data;
        if (typeof data === "string") {
            try {
                data = JSON.parse(evt.data.replace(/\n/g, "\\n").replace(/\r/g, "\\r").replace(/\t/g, "\\t"));
            } catch (e) {
                alertError({title: "Invalid data received", message: evt.data});
            }
        }
        if (data && typeof data === "object") {
            if (data.id) {
                if (this.queue[data.id]) {
                    this.queue[data.id](data);
                }
            } else {
                this._onMsg(data);
            }
        }
    }
};

export default new Ws;
