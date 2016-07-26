<?php

$container = $app->getContainer();

$settings = $container->get('settings');

$app->add(new RKA\Middleware\IpAddress(
    $settings['rka']['check_proxy_headers'],
    $settings['rka']['trusted_proxies']
));
