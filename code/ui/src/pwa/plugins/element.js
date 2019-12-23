import Vue from 'vue'
import {Loading, Button, Card, Col, Row, Form, Input, ButtonGroup, Divider, Dialog, Progress, Message, Upload, Notification} from 'element-ui'
import lang from 'element-ui/lib/locale/lang/en'
import locale from 'element-ui/lib/locale'

locale.use(lang);

[Loading, Button, ButtonGroup, Col, Row, Form, Input, Divider, Card, Dialog, Progress, Message, Upload, Notification].forEach((v) => {
    Vue.use(v);
});

Vue.prototype.$notify = Notification;
Vue.prototype.$message = Message;

import './theme.scss'
