<?php
return [
    'settings' => [

        'displayErrorDetails' => true, // set to false in production
        'addContentLengthHeader' => false, // Allow the web server to send the content-length header

        // RKA IP Address middleware
        'rka' => [
            'check_proxy_headers' => false,
            'trusted_proxies' => [],
        ],

        // Monolog settings
        'devices' => [
            'name' => 'espurna-update-server-devices',
            'path' => __DIR__ . '/../logs/devices.log',
        ],

        // Database
        'database' => [
            'type' => 'json',
            'path' => __DIR__ . '/../data/versions.json',
        ],

    ],
];
