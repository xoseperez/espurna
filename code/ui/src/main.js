import Vue from "vue";


let App;

// #!if ENV === 'production'
import SingleApp from "./App.vue";
App = SingleApp;
// #!else
import './pwa/registerServiceWorker'
import PwaApp from "./pwa/App.vue";
import './pwa/plugins/element.js'
App = PwaApp;
// #!endif


Vue.config.productionTip = false;

new Vue({
  render: (h) => h(App)
}).$mount("#app");