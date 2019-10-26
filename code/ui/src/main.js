import Vue from "vue";
import App from "./App.vue";

// # if process.env.NODE_ENV !== 'production'
import './registerServiceWorker'
// # endif

Vue.config.productionTip = false;

new Vue({
  render: (h) => h(App)
}).$mount("#app");