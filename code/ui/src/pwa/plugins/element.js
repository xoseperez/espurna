import Vue from 'vue'
import {Button, Card, Col, Row, Form, Input, ButtonGroup, Divider} from 'element-ui'
import lang from 'element-ui/lib/locale/lang/en'
import locale from 'element-ui/lib/locale'

locale.use(lang);

[Button, ButtonGroup, Col, Row, Form, Input, Divider, Card].forEach((v) => {
    Vue.use(v);
});

import './theme.scss'
