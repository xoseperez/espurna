<?php

// Check update entry point
$app->get('/{model}/{current}', function($request, $response, $args) {

    $found = false;
    $model = $request->getAttribute('model');
    $current = $request->getAttribute('current');

    foreach ($this->get('data') as $version) {

        if (($model == $version['origin']['model'])
            && (($version['origin']['min'] == '*' || version_compare($version['origin']['min'], $current, '<=')))
            && (($version['origin']['max'] == '*' || version_compare($version['origin']['max'], $current, '>=')))) {

            $response->getBody()->write(stripslashes(json_encode($version['target'])));
            $found = true;
            break;

        }
    };

    if (!$found) {
        $response->getBody()->write("{}");
    }

    $this->get('devices')->info(
        "from:"
        . $request->getAttribute('ip_address')
        . " model:$model version:$current update:"
        . ($found ? $version['target']['version'] : "none")
    );

    return $response;

});
