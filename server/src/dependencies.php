<?php
// DIC configuration

$container = $app->getContainer();

// view renderer
// Register component on container
$container['view'] = function ($container) {
    $settings = $container->get('settings')['renderer'];
    $view = new \Slim\Views\Twig($settings['template_path'], [
        'cache' => $settings['cache_path']
    ]);
    $view->addExtension(new \Slim\Views\TwigExtension(
        $container['router'],
        $container['request']->getUri()
    ));

    return $view;
};

// monolog
$container['devices'] = function ($container) {

    $settings = $container->get('settings')['devices'];

    $dateFormat = "[Y/m/d H:i:s]";
    $output = "%datetime% %message%\n";
    $formatter = new Monolog\Formatter\LineFormatter($output, $dateFormat);
    $stream = new Monolog\Handler\StreamHandler($settings['path'], Monolog\Logger::INFO);
    $stream->setFormatter($formatter);

    $logger = new Monolog\Logger($settings['name']);
    $logger->pushHandler($stream);

    return $logger;

};

// version database
$container['data'] = function($container) {
    $settings = $container->get('settings')['database'];
    $json_data = file_get_contents($settings['path']);
    $data = json_decode($json_data, true);
    return $data;
};
