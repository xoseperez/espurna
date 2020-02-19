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
    if (binding.value) {
      el.classList.add("loading");
      el.setAttribute("disabled", "disabled");
    } else {
      el.classList.remove("loading");
      el.removeAttribute("disabled");
    }
  },
  unbind(el) {
    el.classList.remove("loading");
  },
});

Vue.config.productionTip = false;

new Vue({
  render: (h) => h(App)
}).$mount("#app");
