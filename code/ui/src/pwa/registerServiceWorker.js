/* eslint-disable no-console */
import {register} from "register-service-worker";
import {alertError} from "../common/notification";

//TODO add a version system

register(`${process.env.BASE_URL}service-worker.js`, {
    ready() {
    },
    registered() {
    },
    cached() {
    },
    updatefound(event) {
    },
    updated(event) {
    },
    offline() {
    },
    error(error) {
        alertError({title: "Cannot register service worker", message: error});
    }
});
