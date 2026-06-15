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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_landmark_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_landmark_pack";

const materials = {
	stucco: mat("sunwashed_stucco", 0xf0e6d1, 0.64, 0.0, 0.0, null),
	coral: mat("coral_ceramic", 0xe75f52, 0.5, 0.0, 0.0, null),
	aquaGlass: mat("aqua_low_e_glass", 0x55c9d8, 0.18, 0.0, 0.28, null),
	nightGlass: mat("deep_blue_glass", 0x101a24, 0.22, 0.0, 0.35, null),
	concrete: mat("salt_air_concrete", 0xa49d8f, 0.86, 0.0, 0.0, null),
	steel: mat("brushed_weathered_steel", 0x6f7478, 0.34, 0.5, 0.0, null),
	neon: mat("warm_cyan_neon", 0x7eeeff, 0.28, 0.0, 0.0, 0x4beaff),
	amber: mat("sunset_amber_light", 0xffb84d, 0.32, 0.0, 0.0, 0xff8c23),
	black: mat("matte_sign_black", 0x111213, 0.72, 0.0, 0.0, null),
	palm: mat("palm_frond_satin", 0x2f6c3e, 0.76, 0.0, 0.0, null),
	trunk: mat("fibrous_palm_trunk", 0x7a5032, 0.82, 0.0, 0.0, null)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaLandmarkPack";
pack.userData.provenance = "Original Three.js-authored Florida-inspired model pack; no commercial-game source assets.";
scene.add(pack);

buildDecoHotel(pack, materials);
buildGlassCondo(pack, materials);
buildRouteSign(pack, materials);
buildPalms(pack, materials);
buildPromenadeLights(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_asset = true;
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

function mat(name, color, roughness, metalness, clearcoat, emissive) {
	const material = new THREE.MeshPhysicalMaterial({
		name,
		color,
		roughness,
		metalness,
		clearcoat,
		clearcoatRoughness: 0.44,
		emissive: emissive ?? 0x000000,
		emissiveIntensity: emissive === null ? 0 : 1.35
	});
	material.userData.godot_hint = "Three.js-authored original Florida landmark material";
	return material;
}

function buildDecoHotel(root, mats) {
	const hotel = new THREE.Group();
	hotel.name = "deco_coast_hotel";
	hotel.position.set(-26, 0, 0);
	root.add(hotel);

	addBox(hotel, "hotel_podium", [28, 8, 18], [0, 4, 0], mats.stucco);
	addBox(hotel, "hotel_main_tower", [18, 52, 14], [0, 34, 0], mats.stucco);
	addBox(hotel, "hotel_stepped_crown", [12, 8, 11], [0, 64, 0], mats.coral);
	addBox(hotel, "hotel_roof_band", [22, 2, 16], [0, 61, 0], mats.neon);

	for (let floor = 0; floor < 10; floor += 1) {
		for (const x of [-5.4, 0, 5.4]) {
			addBox(hotel, `hotel_window_${floor}_${x}`, [2.1, 2.8, 0.45], [x, 14 + floor * 4.4, -7.25], mats.aquaGlass);
		}
	}
	for (const x of [-10, 10]) {
		addBox(hotel, `hotel_entry_column_${x}`, [1.8, 7.5, 1.8], [x, 3.8, -9.3], mats.coral);
	}
	addBox(hotel, "hotel_entry_canopy", [18, 1.3, 5.2], [0, 8.8, -10.6], mats.neon);
}

function buildGlassCondo(root, mats) {
	const condo = new THREE.Group();
	condo.name = "bay_glass_condo";
	condo.position.set(16, 0, 8);
	condo.rotation.y = -0.22;
	root.add(condo);

	addBox(condo, "condo_podium", [22, 10, 20], [0, 5, 0], mats.concrete);
	addBox(condo, "condo_tower_a", [11, 72, 14], [-4.5, 46, 0], mats.nightGlass);
	addBox(condo, "condo_tower_b", [10, 58, 12], [7.2, 39, 2], mats.aquaGlass);
	addBox(condo, "condo_skybridge", [17, 4, 6], [2, 56, 0.5], mats.stucco);

	for (let floor = 0; floor < 12; floor += 1) {
		addBox(condo, `condo_balcony_a_${floor}`, [12.2, 0.45, 1.2], [-4.5, 15 + floor * 4.6, -7.5], mats.steel);
	}
	for (let floor = 0; floor < 9; floor += 1) {
		addBox(condo, `condo_balcony_b_${floor}`, [11, 0.45, 1.2], [7.2, 15 + floor * 4.5, -4.9], mats.steel);
	}
	addCylinder(condo, "condo_roof_mast", 0.32, 18, [-4.5, 91, 0], mats.neon);
}

function buildRouteSign(root, mats) {
	const sign = new THREE.Group();
	sign.name = "original_state_route_sign";
	sign.position.set(0, 0, -28);
	sign.rotation.y = 0.18;
	root.add(sign);

	for (const x of [-5.5, 5.5]) {
		addCylinder(sign, `route_sign_pole_${x}`, 0.34, 15, [x, 7.5, 0], mats.steel);
	}
	addBox(sign, "route_sign_panel", [20, 7.8, 0.8], [0, 17, 0], mats.black);
	addBox(sign, "route_sign_face", [18.2, 5.8, 0.5], [0, 17.15, -0.48], mats.coral);
	addBox(sign, "route_sign_neon_line", [15.5, 0.5, 0.72], [0, 18.9, -0.82], mats.neon);
	addBox(sign, "route_sign_bottom_line", [11.5, 0.45, 0.72], [0, 15.5, -0.82], mats.amber);
}

function buildPalms(root, mats) {
	const palmPositions = [
		[-42, 0, -16],
		[-36, 0, 18],
		[36, 0, -12],
		[42, 0, 18],
		[3, 0, 26]
	];
	for (let i = 0; i < palmPositions.length; i += 1) {
		const palm = new THREE.Group();
		palm.name = `wind_bent_palm_${i}`;
		palm.position.set(...palmPositions[i]);
		palm.rotation.y = i * 0.7;
		root.add(palm);

		const trunk = addCylinder(palm, "palm_trunk", 0.65, 16, [0, 8, 0], mats.trunk);
		trunk.rotation.z = 0.13;
		for (let j = 0; j < 7; j += 1) {
			const frond = addBox(palm, `palm_frond_${j}`, [1.0, 0.3, 8.8], [0, 16.5, 0], mats.palm);
			frond.rotation.y = (Math.PI * 2 * j) / 7;
			frond.rotation.x = 0.42;
			frond.translateZ(3.2);
		}
	}
}

function buildPromenadeLights(root, mats) {
	const base = new THREE.Group();
	base.name = "promenade_light_row";
	base.position.set(0, 0, 18);
	root.add(base);

	for (let i = 0; i < 5; i += 1) {
		const x = -24 + i * 12;
		addCylinder(base, `promenade_pole_${i}`, 0.18, 8.5, [x, 4.25, 0], mats.steel);
		addBox(base, `promenade_arm_${i}`, [4.5, 0.28, 0.28], [x + 2.2, 8.35, 0], mats.steel);
		addSphere(base, `promenade_globe_${i}`, 0.85, [x + 4.6, 7.85, 0], mats.amber);
	}
}

function addBox(parent, name, size, position, material) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), material);
	return place(parent, mesh, name, position);
}

function addCylinder(parent, name, radius, height, position, material) {
	const mesh = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, height, 20), material);
	return place(parent, mesh, name, position);
}

function addSphere(parent, name, radius, position, material) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 20, 14), material);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
