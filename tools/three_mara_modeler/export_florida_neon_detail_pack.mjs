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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_neon_detail_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_neon_detail_pack";

const materials = {
	darkGlass: makeMaterial("rain_black_glass", 0x101219, 0.18, 0.0, 0.42, null),
	wetAsphalt: makeMaterial("wet_charcoal_asphalt", 0x1b2024, 0.34, 0.0, 0.18, null),
	polishedConcrete: makeMaterial("polished_shell_concrete", 0xbcb3a2, 0.52, 0.0, 0.0, null),
	brushedSteel: makeMaterial("brushed_stainless_steel", 0x85898b, 0.28, 0.62, 0.0, null),
	chrome: makeMaterial("mirror_chrome_trim", 0xb8c3ca, 0.18, 0.82, 0.0, null),
	cyanNeon: makeMaterial("cyan_neon_tube", 0x7df8ff, 0.18, 0.0, 0.0, 0x35f5ff, 2.4),
	pinkNeon: makeMaterial("pink_neon_tube", 0xff5b9a, 0.2, 0.0, 0.0, 0xff327f, 2.25),
	amberNeon: makeMaterial("amber_neon_tube", 0xffba55, 0.22, 0.0, 0.0, 0xff8c26, 2.0),
	greenNeon: makeMaterial("aqua_green_neon_tube", 0x58ffb0, 0.2, 0.0, 0.0, 0x33ff9b, 2.0),
	warmInterior: makeMaterial("warm_lobby_interior", 0xffc980, 0.32, 0.0, 0.0, 0xffa54a, 1.4)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaNeonDetailPack";
pack.userData.provenance = "Original Three.js-authored Florida neon/street detail prop pack; no commercial-game source assets.";
scene.add(pack);

buildWetStreetCorner(pack, materials);
buildNeonHotelCanopy(pack, materials);
buildRooftopSign(pack, materials);
buildStorefrontRow(pack, materials);
buildArtLightPoles(pack, materials);
buildPoolDeck(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_neon_detail = true;
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

function makeMaterial(name, color, roughness, metalness, clearcoat, emissive, emissiveIntensity = 0) {
	const mat = new THREE.MeshPhysicalMaterial({
		name,
		color,
		roughness,
		metalness,
		clearcoat,
		clearcoatRoughness: 0.32,
		emissive: emissive ?? 0x000000,
		emissiveIntensity
	});
	mat.userData.godot_hint = "Three.js-authored original Florida neon detail material";
	return mat;
}

function buildWetStreetCorner(root, mats) {
	const corner = new THREE.Group();
	corner.name = "wet_street_corner";
	root.add(corner);

	addBox(corner, "wet_road_plate", [72, 0.45, 38], [0, 0.22, 0], mats.wetAsphalt);
	addBox(corner, "shell_sidewalk_front", [72, 0.6, 9], [0, 0.7, -23], mats.polishedConcrete);
	addBox(corner, "shell_sidewalk_side", [11, 0.6, 38], [-41.5, 0.7, 0], mats.polishedConcrete);
	for (let i = 0; i < 6; i += 1) {
		addBox(corner, `rain_reflection_cyan_${i}`, [7.5, 0.04, 0.8], [-28 + i * 11.5, 0.5, -6 + (i % 2) * 8], mats.cyanNeon);
		addBox(corner, `rain_reflection_pink_${i}`, [5.4, 0.04, 0.65], [-31 + i * 12, 0.52, 5 + (i % 2) * 7], mats.pinkNeon);
	}
}

function buildNeonHotelCanopy(root, mats) {
	const canopy = new THREE.Group();
	canopy.name = "boutique_hotel_neon_canopy";
	canopy.position.set(-18, 0, -18);
	root.add(canopy);

	addBox(canopy, "hotel_glass_lobby", [28, 13, 8], [0, 7, 0], mats.darkGlass);
	addBox(canopy, "hotel_canopy_slab", [34, 1.2, 10], [0, 13.2, -3.2], mats.chrome);
	addBox(canopy, "hotel_cyan_front_tube", [30, 0.5, 0.55], [0, 14.1, -8.45], mats.cyanNeon);
	addBox(canopy, "hotel_pink_lower_tube", [25, 0.42, 0.55], [0, 11.8, -8.5], mats.pinkNeon);
	for (let i = 0; i < 5; i += 1) {
		addBox(canopy, `lobby_glow_panel_${i}`, [3.2, 5.4, 0.35], [-10 + i * 5, 7.0, -4.25], mats.warmInterior);
	}
}

function buildRooftopSign(root, mats) {
	const sign = new THREE.Group();
	sign.name = "rooftop_neon_sign";
	sign.position.set(19, 0, -16);
	sign.rotation.y = -0.18;
	root.add(sign);

	for (const x of [-8, 8]) {
		addCylinder(sign, `sign_truss_${x}`, 0.22, 20, [x, 10, 0], mats.brushedSteel);
	}
	addBox(sign, "sign_backing", [22, 8, 0.75], [0, 22, 0], mats.darkGlass);
	addBox(sign, "sign_top_cyan", [18, 0.55, 1.0], [0, 24.7, -0.75], mats.cyanNeon);
	addBox(sign, "sign_mid_pink", [15, 0.55, 1.0], [0, 22, -0.78], mats.pinkNeon);
	addBox(sign, "sign_low_amber", [11, 0.55, 1.0], [0, 19.2, -0.8], mats.amberNeon);
	addBox(sign, "sign_side_green", [0.55, 6.5, 1.0], [-10, 22, -0.82], mats.greenNeon);
	addBox(sign, "sign_side_green_r", [0.55, 6.5, 1.0], [10, 22, -0.82], mats.greenNeon);
}

function buildStorefrontRow(root, mats) {
	const row = new THREE.Group();
	row.name = "neon_storefront_row";
	row.position.set(16, 0, 12);
	root.add(row);

	for (let i = 0; i < 4; i += 1) {
		const x = -18 + i * 12;
		addBox(row, `store_${i}_body`, [10, 8, 8], [x, 4.4, 0], mats.polishedConcrete);
		addBox(row, `store_${i}_glass`, [7.2, 5.2, 0.45], [x, 4.5, -4.25], mats.darkGlass);
		addBox(row, `store_${i}_awning`, [9.6, 0.7, 2.1], [x, 8.7, -5.2], i % 2 === 0 ? mats.cyanNeon : mats.pinkNeon);
		addBox(row, `store_${i}_interior`, [5.4, 3.8, 0.35], [x, 4.4, -4.55], mats.warmInterior);
	}
}

function buildArtLightPoles(root, mats) {
	const lights = new THREE.Group();
	lights.name = "art_light_poles";
	lights.position.set(-4, 0, 18);
	root.add(lights);

	for (let i = 0; i < 7; i += 1) {
		const x = -30 + i * 10;
		addCylinder(lights, `art_pole_${i}`, 0.16, 9, [x, 4.8, 0], mats.brushedSteel);
		addBox(lights, `art_pole_arm_${i}`, [3.0, 0.22, 0.22], [x + 1.5, 9.2, 0], mats.brushedSteel);
		addSphere(lights, `art_pole_globe_${i}`, 0.68, [x + 3.2, 8.8, 0], i % 2 === 0 ? mats.amberNeon : mats.cyanNeon);
	}
}

function buildPoolDeck(root, mats) {
	const deck = new THREE.Group();
	deck.name = "glowing_pool_deck";
	deck.position.set(30, 0, 20);
	deck.rotation.y = 0.28;
	root.add(deck);

	addBox(deck, "pool_deck_slab", [24, 0.8, 16], [0, 1.0, 0], mats.polishedConcrete);
	addBox(deck, "pool_water_glow", [16, 0.18, 8], [0, 1.55, -1], mats.cyanNeon);
	for (const x of [-9, -3, 3, 9]) {
		addBox(deck, `pool_lounger_${x}`, [3.6, 0.42, 8], [x, 1.8, 5.0], mats.white ?? mats.polishedConcrete);
	}
	addBox(deck, "pool_bar_glow", [16, 1.2, 2.2], [0, 2.0, -8.5], mats.amberNeon);
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
