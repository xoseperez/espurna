export default (color_1, color_2, weight) => {
    function d2h(d) { return d.toString(16); }  // convert a decimal value to hex
    function h2d(h) { return parseInt(h, 16); } // convert a hex value to decimal

    color_1 = color_1.indexOf("#") === 0 ? color_1.substr(1) : color_1;
    color_2 = color_2.indexOf("#") === 0 ? color_2.substr(1) : color_2;
    weight = (typeof(weight) !== "undefined") ? weight : 50; // set the weight to 50%, if that argument is omitted

    let color = "#";

    for(let i = 0; i <= 5; i += 2) { // loop through each of the 3 hex pairs—red, green, and blue
        let v1 = h2d(color_1.substr(i, 2)), // extract the current pairs
            v2 = h2d(color_2.substr(i, 2)),

            // combine the current pairs from each source color, according to the specified weight
            val = d2h(Math.floor(v2 + (v1 - v2) * (weight / 100.0)));

        while(val.length < 2) { val = "0" + val; } // prepend a '0' if val results in a single digit

        color += val; // concatenate val to our new color string
    }

    return color; // PROFIT!
};