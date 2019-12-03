import Vue from "vue";

let App;

// #if process.env.NODE_ENV !== 'production'
import './pwa/registerServiceWorker'
import PwaApp from "./pwa/App.vue";
App = PwaApp;
// #endif

// #if process.env.NODE_ENV === 'production'
import SingleApp from "./App.vue";
App = SingleApp;
// #endif

Vue.config.productionTip = false;

new Vue({
  render: (h) => h(App)
}).$mount("#app");