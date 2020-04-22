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

import "./directives/loading";

// #!if ENV === "development"
/* eslint-disable no-console */
Vue.prototype.$log = console.log;
/* eslint-enable */
// #!endif

Vue.config.productionTip = false;

new Vue({
    render: (h) => h(App)
}).$mount("#app");
