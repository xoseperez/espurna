<?php

// Routes

$app->get('/{model}/{current}', function($request, $response, $args) {

    $found = false;
    $model = $request->getAttribute('model');
    $current = $request->getAttribute('current');

    foreach ($this->get('data') as $version) {

        if (($model == $version['model'])
            && (($version['firmware']['min'] == "*" || version_compare($version['firmware']['min'], $current, "<=")))
            && (($version['firmware']['max'] == "*" || version_compare($version['firmware']['max'], $current, ">")))) {

            $response->getBody()->write(stripslashes(json_encode(array(
                'action' => 'update',
                'target' => $version["target"]
            ))));

            $found = true;
            break;

        }
    };

    if (!$found) {
        $response->getBody()->write(stripslashes(json_encode(array(
            'action' => 'none',
        ))));
    }

    return $response;

});
