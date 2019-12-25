export default (address, opts) => {
    return new Promise((resolve, fail) => {
            setTimeout(() => {
                const url = new URL(address);
                if (url.hostname === '127.0.0.1') {
                    switch (url.pathname) {
                        case '/discover':
                            resolve(
                                {
                                    ok: true, json() {
                                        return {
                                            app: "Espurna",
                                            version: "2.0.1",
                                            device: "ITEAD_SONOFF_SV",
                                            hostname: "Espurna device",
                                            description: "A mocked espurna device",
                                            free_size: 504080,
                                            wifi: "MyWifi",
                                            rssi: -(Math.random() * 50 + 40)
                                        }
                                    }
                                });
                            break;
                        case '/auth':
                            if (opts.headers.get('Authorization') === 'Basic ' + btoa('admin:fibonacci')) {
                                resolve({
                                    ok: true
                                })
                            } else {
                                fail("Authentication failed");
                            }
                    }
                } else {
                    fail("Ip address for mocked device is 127.0.0.1");
                }
            }, 1000)
        }
    )
}
