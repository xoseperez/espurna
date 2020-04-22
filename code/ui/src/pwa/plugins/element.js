import Vue from "vue";
import {Loading, Button, Card, Col, Row, Form, Input, ButtonGroup, Divider, Dialog, Progress, Message, Upload, Notification, MessageBox} from "element-ui";
import lang from "element-ui/lib/locale/lang/en";
import locale from "element-ui/lib/locale";

locale.use(lang);

[Button, ButtonGroup, Col, Row, Form, Input, Divider, Card, Dialog, Progress, Upload].forEach((v) => {
    Vue.use(v);
});

Vue.confirm = MessageBox.confirm;
Vue.alert = MessageBox.alert;
Vue.notify = Notification;
Vue.message = Message;

Vue.prototype.$notify = Notification;
Vue.prototype.$message = Message;
Vue.prototype.$confirm = MessageBox.confirm;
Vue.prototype.$alert = MessageBox.alert;

import "./theme.scss";
