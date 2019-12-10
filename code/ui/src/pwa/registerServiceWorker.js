/* eslint-disable no-console */
import {register} from "register-service-worker"

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
        console.log('Error during service worker registration:' + error);
    }
});