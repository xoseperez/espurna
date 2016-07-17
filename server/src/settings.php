<?php
return [
    'settings' => [
        'displayErrorDetails' => true, // set to false in production
        'addContentLengthHeader' => false, // Allow the web server to send the content-length header

        // Renderer settings
        'renderer' => [
            'template_path' => __DIR__ . '/../templates/',
            'cache_path' => __DIR__ . '/../cache/',
        ],

        // Monolog settings
        'logger' => [
            'name' => 'espurna-update-server',
            'path' => __DIR__ . '/../logs/app.log',
        ],

        // Database
        'database' => [
            'type' => 'json',
            'path' => __DIR__ . '/../data/versions.json',
        ],

    ],
];
