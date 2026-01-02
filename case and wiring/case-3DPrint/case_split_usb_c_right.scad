include <../include.scad>

esp_offset = [127.95, 0, -29];
esp_rotation = [0, 0, 180];


module hole() {
    cz = 1.2;
    h = 15;
    cxy = 13.98;
    size = [cxy, cxy, h];
    z1 = -h / 2;
    sizeb = [cxy + 1, cxy, cz];
    sizet = [cxy + 5, cxy + 5, 4];

    z = [0, 0, 1];
    z1 = -h / 2;
    z2 = -cz * 1.5;
    translate(z1 * z)cube(size, center = true);
    translate(z2 * z)cube(sizeb, center = true);
    translate(2 * z)cube(sizet, center = true);
}

use <../case-Lasercut/esp32mount.scad>

difference() {
    import("bifrost-led-matrix-case-model_files/case_split_right_usb_c.stl");
    translate(esp_offset)rotate(esp_rotation)translate([-x / 2 - 3 - 10, -y / 2 - 3, 1])cube([10, y + 6, 8]);

    translate([112, -29, -30])rotate([00, 00, 180])#union() {translate([-1.5, 0, 1])cube([16.5, 5.5, 10]);
        cube([13.5, 5.5, 10]);
    }
    for (i = [0, 1, 2])translate([70 + 20 * i, 40.3, -16])rotate([-90, 0, 0])hole();

}
translate(esp_offset)rotate(esp_rotation)esp32mount();
