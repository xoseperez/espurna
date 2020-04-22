import {mount} from "@vue/test-utils";
import App from "../../src/App";

//TODO replace alerts with custom code
window.alert = (msg) => {
    console.log(msg);
};

function timeout(ms, cb) {
    return new Promise(resolve => setTimeout(() => {
        if (cb) {cb();}
        resolve();
    }, ms));
}

describe("App", () => {

    it("doesn't throw any error", async () => {
        const app = mount(App);

        expect(app.vm.isLoaded).toBe(false);

        await timeout(1000);

        expect(app.vm.isLoaded).toBe(true);

        let i = 0;

        app.findAll(".menu .inner .list li:not(.separator) a").wrappers.forEach((a) => {
            timeout(i, () => {
                a.element.click();
            });
            i += 100;
        });

        await timeout(i);
    });


});
