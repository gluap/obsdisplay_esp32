x = 46.6;
y = 23.7;
$fn = 20;
kk = (13 - 3) / 2;
usb_plughole_diameter = 8;
usb_plughole_depth = 8;
usb_opening_width = 10;
usb_opening_height = 4;

module esp32mount(extra_h = 0) {
    // extra_h thickens mounting plate for wooden case mounting.
    difference() {
        union() {
            // The mounting plate
            translate([-x / 2 - 3 - 10, -y / 2 - 3, -extra_h])cube([x + 18, y + 6, 3 + extra_h]);
            // screw mounts
            for (xx = [-x / 2, x / 2]) for (yy = [-y / 2, y / 2]) {
                translate([xx, yy, 3])cylinder(d = 5, h = 3.2);
            }
            // block around USB-C
            translate([-x / 2 - 3 - 10, -y / 2 - 3, 3])cube([10, y + 6, 7]);
        }
        // the screw holes
        for (xx = [-x / 2, x / 2]) for (yy = [-y / 2, y / 2]) {
            translate([xx, yy, 0])cylinder(d = 3, h = 8);
        }
        // cutaway from screw holes for ESP32 components
        translate([-x / 2 - 4, -y / 2 + 1.55, 3.01])cube([10, y - 3.1, 4]);

        // usb-c plug opening
        translate([-x / 2 - 6, -usb_opening_width / 2, 5 - usb_opening_height / 2])cube([10, usb_opening_width,
            usb_opening_height]);

        // pill-shaped cavity for the plug
        hull() {
            for (ff = [-kk, kk]) {
                translate([-x / 2 - usb_plughole_depth + 2, ff, 5.25])rotate([0, -90, 0])cylinder(d =
                usb_plughole_diameter, h = usb_plughole_depth);
            }
        }
        // small cutaway for better fit in lasercut case.
        if (extra_h > 0) translate([-x / 2 - 3 - 10, -y / 2 - 3, -extra_h])cube([0.5, y + 6, extra_h]);
    }
}
