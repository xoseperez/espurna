import Vue from "vue";


Vue.directive("loading", {
    bind(el, binding) {
        if (binding.value) {
            el.classList.add("loading");
            el.setAttribute("disabled", "disabled");
        } else {
            el.classList.remove("loading");
            el.removeAttribute("disabled");
        }
    },
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
