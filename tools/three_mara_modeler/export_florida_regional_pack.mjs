import { mkdir, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import * as THREE from "three";
import { GLTFExporter } from "three/examples/jsm/exporters/GLTFExporter.js";

globalThis.FileReader = class {
	constructor() {
		this.result = null;
		this.onloadend = null;
	}

	readAsArrayBuffer(blob) {
		blob.arrayBuffer().then((buffer) => {
			this.result = buffer;
			this.onloadend?.();
		});
	}

	readAsDataURL(blob) {
		blob.arrayBuffer().then((buffer) => {
			const base64 = Buffer.from(buffer).toString("base64");
			this.result = `data:${blob.type || "application/octet-stream"};base64,${base64}`;
			this.onloadend?.();
		});
	}
};

const __dirname = dirname(fileURLToPath(import.meta.url));
const repoRoot = resolve(__dirname, "../..");
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_regional_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_regional_pack";

const materials = {
	concrete: material("sun_baked_concrete", 0xb8b0a0, 0.86, 0.0, 0.0, null, 0),
	asphalt: material("faded_runway_asphalt", 0x25282c, 0.82, 0.0, 0.0, null, 0),
	steel: material("coastal_galvanized_steel", 0x757b7e, 0.38, 0.58, 0.0, null, 0),
	containerBlue: material("container_blue", 0x2f6fa6, 0.62, 0.0, 0.0, null, 0),
	containerCoral: material("container_coral", 0xc95d54, 0.64, 0.0, 0.0, null, 0),
	containerSand: material("container_sand", 0xc6aa70, 0.68, 0.0, 0.0, null, 0),
	wetlandWood: material("silvered_boardwalk_wood", 0x8b765c, 0.88, 0.0, 0.0, null, 0),
	mangrove: material("mangrove_canopy", 0x244b2f, 0.82, 0.0, 0.0, null, 0),
	water: material("shallow_turquoise_water", 0x39b8c4, 0.25, 0.0, 0.35, 0x1dd7dd, 0.55),
	resortWhite: material("regional_resort_white", 0xefe7d6, 0.6, 0.0, 0.0, null, 0),
	orangeLight: material("runway_amber_light", 0xffad4a, 0.28, 0.0, 0.0, 0xff8a22, 1.8),
	cyanLight: material("pier_cyan_light", 0x7df7ff, 0.24, 0.0, 0.0, 0x35ecff, 1.6),
	rocketWhite: material("rocket_white_ceramic", 0xf4f0e6, 0.5, 0.0, 0.0, null, 0),
	rocketRed: material("rocket_warning_red", 0xc83c33, 0.48, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaRegionalPack";
pack.userData.provenance = "Original Three.js-authored regional Florida destination pack; no commercial-game source assets.";
scene.add(pack);

buildPortTerminal(pack, materials);
buildAirportApron(pack, materials);
buildWetlandBoardwalk(pack, materials);
buildKeyResortPier(pack, materials);
buildSpaceCoastPad(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_regional = true;
});

const exporter = new GLTFExporter();
const glb = await exporter.parseAsync(scene, {
	binary: true,
	onlyVisible: true,
	trs: false
});

await mkdir(dirname(outputPath), { recursive: true });
await writeFile(outputPath, Buffer.from(glb));
console.log(`exported ${outputPath}`);

function material(name, color, roughness, metalness, clearcoat, emissive, emissiveIntensity) {
	const mat = new THREE.MeshPhysicalMaterial({
		name,
		color,
		roughness,
		metalness,
		clearcoat,
		clearcoatRoughness: 0.42,
		emissive: emissive ?? 0x000000,
		emissiveIntensity
	});
	mat.userData.godot_hint = "Three.js-authored original regional Florida material";
	return mat;
}

function buildPortTerminal(root, mats) {
	const port = new THREE.Group();
	port.name = "coastal_container_port";
	port.position.set(-70, 0, 0);
	root.add(port);

	addBox(port, "port_quay", [58, 1.1, 28], [0, 0.55, 0], mats.concrete);
	addBox(port, "port_water_berth", [58, 0.18, 18], [0, 0.78, 23], mats.water);
	for (let i = 0; i < 18; i += 1) {
		const row = Math.floor(i / 6);
		const col = i % 6;
		const color = [mats.containerBlue, mats.containerCoral, mats.containerSand][i % 3];
		addBox(port, `shipping_container_${i}`, [7.2, 2.8, 3.0], [-22 + col * 8.8, 2.0 + row * 2.9, -8 + row * 4.2], color);
	}
	for (const x of [-20, 8]) {
		addBox(port, `gantry_leg_l_${x}`, [0.9, 18, 0.9], [x - 7, 9.5, 7], mats.steel);
		addBox(port, `gantry_leg_r_${x}`, [0.9, 18, 0.9], [x + 7, 9.5, 7], mats.steel);
		addBox(port, `gantry_beam_${x}`, [17, 1.2, 1.0], [x, 18.8, 7], mats.steel);
		addBox(port, `gantry_trolley_${x}`, [3.8, 2.2, 2.6], [x + 2, 16.8, 7], mats.orangeLight);
	}
}

function buildAirportApron(root, mats) {
	const airport = new THREE.Group();
	airport.name = "regional_airport_apron";
	airport.position.set(0, 0, -54);
	airport.rotation.y = 0.1;
	root.add(airport);

	addBox(airport, "runway_strip", [86, 0.45, 14], [0, 0.32, 0], mats.asphalt);
	addBox(airport, "taxiway_strip", [44, 0.42, 10], [-8, 0.36, 18], mats.asphalt);
	addBox(airport, "terminal_bar", [32, 8, 10], [22, 4.4, 28], mats.resortWhite);
	addBox(airport, "terminal_glass", [28, 4.4, 0.5], [22, 5.0, 22.7], mats.water);
	for (let i = 0; i < 10; i += 1) {
		addBox(airport, `runway_marker_${i}`, [4.2, 0.08, 0.55], [-38 + i * 8.5, 0.65, 0], mats.orangeLight);
	}
	for (const x of [-18, 3]) {
		buildSmallJet(airport, mats, x, 2.4, 17, x < 0 ? -0.05 : 0.28);
	}
}

function buildSmallJet(parent, mats, x, y, z, yaw) {
	const jet = new THREE.Group();
	jet.name = "regional_jet";
	jet.position.set(x, y, z);
	jet.rotation.y = yaw;
	parent.add(jet);
	addCylinder(jet, "jet_fuselage", 1.2, 13, [0, 0, 0], mats.rocketWhite).rotation.x = Math.PI / 2;
	addBox(jet, "jet_wing", [14, 0.28, 3.2], [0, -0.1, 0], mats.rocketWhite);
	addBox(jet, "jet_tail", [4.0, 3.2, 0.45], [0, 2.1, 5.5], mats.rocketRed);
	addCylinder(jet, "jet_engine_l", 0.45, 1.8, [-4.2, -0.5, 0.5], mats.steel).rotation.x = Math.PI / 2;
	addCylinder(jet, "jet_engine_r", 0.45, 1.8, [4.2, -0.5, 0.5], mats.steel).rotation.x = Math.PI / 2;
}

function buildWetlandBoardwalk(root, mats) {
	const wetland = new THREE.Group();
	wetland.name = "wetland_boardwalk_stop";
	wetland.position.set(58, 0, 6);
	wetland.rotation.y = -0.35;
	root.add(wetland);

	addBox(wetland, "shallow_water_patch", [46, 0.18, 34], [0, 0.28, 0], mats.water);
	for (let i = 0; i < 7; i += 1) {
		addBox(wetland, `boardwalk_section_${i}`, [6.2, 0.7, 4.4], [-20 + i * 6.6, 1.1, Math.sin(i * 0.7) * 5], mats.wetlandWood);
	}
	for (let i = 0; i < 18; i += 1) {
		const x = -21 + (i % 6) * 8.4;
		const z = -13 + Math.floor(i / 6) * 11;
		addCylinder(wetland, `mangrove_trunk_${i}`, 0.42, 5.2, [x, 2.9, z], mats.wetlandWood);
		addSphere(wetland, `mangrove_crown_${i}`, 2.4, [x, 6.2, z], mats.mangrove);
	}
	addBox(wetland, "watch_platform", [11, 1.1, 10], [19, 1.3, 6], mats.wetlandWood);
	addBox(wetland, "watch_roof", [13, 0.8, 12], [19, 8.2, 6], mats.resortWhite);
	for (const x of [14.5, 23.5]) {
		for (const z of [1.5, 10.5]) {
			addCylinder(wetland, `watch_post_${x}_${z}`, 0.28, 7, [x, 4.5, z], mats.wetlandWood);
		}
	}
}

function buildKeyResortPier(root, mats) {
	const resort = new THREE.Group();
	resort.name = "keys_resort_pier";
	resort.position.set(-38, 0, 52);
	resort.rotation.y = -0.18;
	root.add(resort);

	addBox(resort, "sand_key_base", [54, 0.55, 22], [0, 0.3, 0], mats.resortWhite);
	addBox(resort, "pier_walkway", [8, 0.7, 48], [0, 1.0, 20], mats.wetlandWood);
	for (let i = 0; i < 6; i += 1) {
		const side = i % 2 === 0 ? -1 : 1;
		addBox(resort, `overwater_bungalow_${i}`, [9, 4.8, 7], [side * 12, 3.1, 5 + i * 6.5], mats.resortWhite);
		addBox(resort, `bungalow_glow_${i}`, [7, 2.5, 0.5], [side * 12, 3.2, 1.2 + i * 6.5], mats.cyanLight);
	}
	for (let i = 0; i < 9; i += 1) {
		addCylinder(resort, `pier_light_${i}`, 0.14, 3.8, [-3.5 + (i % 2) * 7, 3.0, -2 + i * 5.4], mats.orangeLight);
	}
}

function buildSpaceCoastPad(root, mats) {
	const pad = new THREE.Group();
	pad.name = "space_coast_launch_pad";
	pad.position.set(54, 0, 56);
	pad.rotation.y = 0.28;
	root.add(pad);

	addBox(pad, "launch_pad_slab", [44, 1.2, 34], [0, 0.6, 0], mats.concrete);
	addBox(pad, "service_tower_core", [6, 48, 6], [-12, 24.5, 0], mats.steel);
	for (let y = 9; y <= 42; y += 8) {
		addBox(pad, `service_arm_${y}`, [20, 1.1, 3.2], [-3, y, 0], mats.steel);
	}
	addCylinder(pad, "rocket_body", 2.6, 42, [8, 22.0, 0], mats.rocketWhite);
	addCylinder(pad, "rocket_nose", 0.1, 7.5, [8, 46.8, 0], mats.rocketRed);
	addCylinder(pad, "rocket_flame_glow", 3.2, 7.5, [8, 4.0, 0], mats.orangeLight);
	for (const x of [-17, -7, 18]) {
		addCylinder(pad, `warning_mast_${x}`, 0.22, 12, [x, 6.5, -12], mats.orangeLight);
	}
}

function addBox(parent, name, size, position, mat) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), mat);
	return place(parent, mesh, name, position);
}

function addCylinder(parent, name, radius, height, position, mat) {
	const mesh = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, height, 20), mat);
	return place(parent, mesh, name, position);
}

function addSphere(parent, name, radius, position, mat) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 20, 14), mat);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
