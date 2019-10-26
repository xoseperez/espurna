// #if process.env.NODE_ENV === 'development'
import mockServer from './mock-websocket';
// #endif

let Ws = function () {
};
Ws.prototype = {
    ws: undefined,
    retry(time, args) {
        setTimeout(() => {
            this.connectToUrl.apply(this, args)
        }, time);
    },
    connect(host, cb) {
        // #if process.env.NODE_ENV === 'development'
        if (!host || host.match('localhost')) {
            //Start mocking
            this.ws = mockServer();
            this.ws.onmessage = cb;
            return;
        }
        // #endif

        if (!host.startsWith("http:") && !host.startsWith("https:")) {
            host = "http://" + host;
        }
        this.connectToUrl(new URL(host), cb);
    },
    connectToUrl(url, onMessage) {
        let args = arguments;
        let urls = this.initUrls(url);

        fetch(urls.auth.href, {
            'method': 'GET',
            'cors': true,
            'credentials': 'same-origin'
        }).then((response) => {
            // Failed, retry
            if (response.status !== 200) {
                return this.retry(5000, args);
            }
            // update websock object
            if (this.ws) {
                this.ws.close();
            }
            this.ws = new WebSocket(urls.ws.href);
            this.ws.onmessage = onMessage;
        }).catch((error) => {
            console.log(error);
            return this.retry(5000, args);
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
            this.ws.send(JSON.stringify({config: data}));
    }
};

export default Ws;