import Vue from "vue";


let App;

// #!if PWA
import "./pwa/registerServiceWorker";
import PwaApp from "./pwa/App.vue";
import "./pwa/plugins/element.js";

App = PwaApp;
// #!else
import SingleApp from "./App.vue";

App = SingleApp;

// #!endif

Vue.directive("loading", {
    update(el, binding) {
        if (el.tagName === "BUTTON") {
            el.classList.add("small");
        }
        if (binding.value) {
            el.classList.add("loading");
            if (!binding.oldValue) {
                el.setAttribute("disabled", "disabled");
            }
        } else {
            el.classList.remove("loading");
            if (binding.oldValue) {
                el.removeAttribute("disabled");
            }
        }
    },
    unbind(el) {
        el.classList.remove("loading");
    },
});

// #!if ENV === "development"
/* eslint-disable no-console */
Vue.prototype.$log = console.log;
/* eslint-enable */
// #!endif

Vue.config.productionTip = false;

new Vue({
    render: (h) => h(App)
}).$mount("#app");
