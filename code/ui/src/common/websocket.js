// #!if ENV === 'development'
import mockServer from './mock-websocket';
// #!endif

let Ws = function () {
};
Ws.prototype = {
    ws: undefined,
    pingInterval: null,
    args: null,
    retry(time, tries) {
        setTimeout(() => {
            this.connectToUrl.apply(this, this.args)
        }, time);

        if (tries > 5) {
            clearInterval(this.pingInterval);
            if (window.confirm("Connection lost with the device, click OK to refresh the page")) {
                window.location.reload();
            }
        }
    },
    connect(host, cb) {
        // #!if ENV === 'development'
        if (!host || host.match('127.0.0.1')) {
            //Start mocking
            this.ws = mockServer();
            this.ws.onmessage = cb;
            return;
        }
        // #!endif

        if (!host.startsWith("http:") && !host.startsWith("https:")) {
            host = "http://" + host;
        }
        this.connectToUrl(new URL(host), cb);
    },
    connectToUrl(url, onMessage, tries) {
        tries = tries || 1;

        this.args = Array.prototype.slice.call(arguments);
        let urls = this.initUrls(url);

        fetch(urls.auth.href, {
            'method': 'GET',
            'cors': true,
            'credentials': 'same-origin'
        }).then((response) => {
            // Failed, retry
            if (response.status !== 200) {
                return this.retry(5000, tries + 1);
            }
            // update websock object
            if (this.ws) {
                this.ws.close();
            }
            this.ws = new WebSocket(urls.ws.href);
            this.ws.onmessage = onMessage;
            this.ws.onclose = this.onClose;
            this.ws.onopen = this.onOpen;

        }).catch((error) => {
            console.log(error);
            return this.retry(5000, tries + 1);
        });
    },
    initUrls(root) {
        let urls = {};
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
        return urls;
    },
    sendAction(action, data) {
        if (this.ws)
            this.ws.send(JSON.stringify({action: action, data: data}));
    },
    sendConfig(data) {
        if (this.ws)
            this.ws.send(
                JSON.stringify({config: data},
                    (key, value) => typeof value === 'undefined' ? null : value)
            );
    },
    onClose() {
        this.retry(1000);
    },
    onOpen() {
        this.pingInterval = setInterval(() => { this.sendAction("ping"); }, 30000);
    }
};

export default Ws;
