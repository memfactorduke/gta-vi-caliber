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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_vista_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_vista_pack";

const materials = {
	stone: material("warm_limestone_vista_base", 0xbfae91, 0.82, 0.0, 0.0, null, 0),
	concrete: material("pale_boardwalk_concrete", 0xd0c8b5, 0.78, 0.0, 0.0, null, 0),
	wood: material("salt_weathered_boardwalk_wood", 0x8a6d4c, 0.86, 0.0, 0.0, null, 0),
	bronze: material("brushed_sunset_bronze", 0xb07942, 0.32, 0.7, 0.12, null, 0),
	steel: material("brushed_coastal_steel", 0xaeb8bc, 0.28, 0.82, 0.08, null, 0),
	glass: material("clear_aqua_vista_glass", 0x74d6e2, 0.12, 0.0, 0.35, null, 0),
	aqua: material("aqua_reflection_tile", 0x2fb9c4, 0.26, 0.0, 0.18, 0x22d5e2, 0.45),
	coral: material("coral_wayfinding_accent", 0xe06455, 0.48, 0.0, 0.08, 0xff5a4d, 0.25),
	amber: material("amber_vista_light", 0xffb35f, 0.3, 0.0, 0.0, 0xff9338, 2.2),
	whiteGlow: material("soft_white_vista_glow", 0xfff0d0, 0.26, 0.0, 0.0, 0xfff0d0, 1.45),
	leaf: material("sea_grape_leaf_mass", 0x356d43, 0.78, 0.0, 0.0, null, 0),
	dark: material("charcoal_shadow_trim", 0x1a1f22, 0.64, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaVistaPack";
pack.userData.provenance = "Original Three.js-authored Florida vista and atmosphere pack; no commercial-game source assets.";
scene.add(pack);

buildSunsetOverlook(pack, materials);
buildBoardwalkPergola(pack, materials);
buildSkylineViewfinder(pack, materials);
buildRouteMonument(pack, materials);
buildLaunchCoastBeacon(pack, materials);
buildGulfArtPier(pack, materials);
buildMicroLighting(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_vista = true;
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
		clearcoatRoughness: 0.36,
		emissive: emissive ?? 0x000000,
		emissiveIntensity
	});
	mat.userData.godot_hint = "Three.js-authored original Florida vista material";
	return mat;
}

function buildSunsetOverlook(root, mats) {
	const group = new THREE.Group();
	group.name = "sunset_overlook_deck";
	group.position.set(-50, 0, -34);
	root.add(group);

	addBox(group, "overlook_limestone_plinth", [38, 1.2, 16], [0, 0.6, 0], mats.stone);
	addBox(group, "overlook_reflection_inlay", [30, 0.08, 4.2], [0, 1.24, -4.8], mats.aqua);
	for (let i = 0; i < 9; i += 1) {
		addBox(group, `bronze_sun_ray_${i}`, [0.38, 0.18, 6.5], [-12 + i * 3, 1.42, -4.8], mats.bronze);
	}
	for (let i = 0; i < 7; i += 1) {
		addCylinder(group, `overlook_guard_post_${i}`, 0.16, 2.6, [-15 + i * 5, 2.55, 6.8], mats.steel);
		addBox(group, `overlook_glass_panel_${i}`, [4.2, 1.45, 0.12], [-15 + i * 5, 2.55, 6.8], mats.glass);
	}
	addTorus(group, "overlook_photo_ring", 4.8, 0.16, [0, 5.3, 5.8], mats.coral).rotation.x = Math.PI / 2;
	for (let i = 0; i < 6; i += 1) {
		addCylinder(group, `overlook_uplight_${i}`, 0.28, 0.22, [-15 + i * 6, 1.55, -6.3], mats.amber);
	}
}

function buildBoardwalkPergola(root, mats) {
	const group = new THREE.Group();
	group.name = "coastal_boardwalk_pergola";
	group.position.set(14, 0, -42);
	group.rotation.y = 0.18;
	root.add(group);

	addBox(group, "pergola_boardwalk", [46, 0.62, 10], [0, 0.36, 0], mats.wood);
	for (let i = 0; i < 11; i += 1) {
		addBox(group, `boardwalk_plank_line_${i}`, [0.12, 0.08, 10.2], [-22 + i * 4.4, 0.72, 0], mats.dark);
	}
	for (let i = 0; i < 6; i += 1) {
		const x = -18 + i * 7.2;
		addCylinder(group, `pergola_post_left_${i}`, 0.24, 4.8, [x, 2.75, -4.3], mats.wood);
		addCylinder(group, `pergola_post_right_${i}`, 0.24, 4.8, [x, 2.75, 4.3], mats.wood);
		addBox(group, `pergola_crossbeam_${i}`, [0.5, 0.38, 10.6], [x, 5.2, 0], mats.wood);
		addSphere(group, `pergola_string_light_l_${i}`, 0.22, [x, 4.35, -3.6], mats.whiteGlow);
		addSphere(group, `pergola_string_light_r_${i}`, 0.22, [x, 4.35, 3.6], mats.whiteGlow);
	}
	for (let i = 0; i < 8; i += 1) {
		addSeaGrape(group, mats, -21 + i * 6, 1.0, i % 2 === 0 ? -7.8 : 7.8);
	}
}

function buildSkylineViewfinder(root, mats) {
	const group = new THREE.Group();
	group.name = "skyline_viewfinder_plaza";
	group.position.set(52, 0, -16);
	group.rotation.y = -0.34;
	root.add(group);

	addCylinder(group, "viewfinder_round_plaza", 11.0, 0.85, [0, 0.5, 0], mats.concrete);
	for (let i = 0; i < 12; i += 1) {
		const angle = (i / 12) * Math.PI * 2;
		addCylinder(
			group,
			`viewfinder_edge_light_${i}`,
			0.22,
			0.18,
			[Math.cos(angle) * 9.4, 1.05, Math.sin(angle) * 9.4],
			mats.amber
		);
	}
	for (let i = 0; i < 5; i += 1) {
		const viewer = new THREE.Group();
		viewer.name = `coin_viewfinder_${i}`;
		viewer.position.set(-6 + i * 3, 1.1, -1.4 + (i % 2) * 2.8);
		viewer.rotation.y = -0.25 + i * 0.12;
		group.add(viewer);
		addCylinder(viewer, "viewer_stand", 0.16, 2.3, [0, 1.15, 0], mats.steel);
		addBox(viewer, "viewer_body", [1.2, 0.65, 0.55], [0, 2.45, 0], mats.bronze);
		addCylinder(viewer, "viewer_lens", 0.28, 0.18, [-0.68, 2.45, 0], mats.glass).rotation.z =
			Math.PI / 2;
	}
	for (let i = 0; i < 8; i += 1) {
		addBox(group, `distant_skyline_marker_${i}`, [0.8 + (i % 3) * 0.45, 3 + (i % 4), 0.6], [
			-10 + i * 2.8,
			2.0 + (i % 4) * 0.5,
			8.0
		], mats.glass);
	}
}

function buildRouteMonument(root, mats) {
	const group = new THREE.Group();
	group.name = "original_route_monument";
	group.position.set(-18, 0, 28);
	group.rotation.y = 0.42;
	root.add(group);

	addBox(group, "monument_base", [26, 1.2, 8], [0, 0.7, 0], mats.stone);
	addBox(group, "monument_dark_backplate", [15, 5.4, 0.8], [0, 3.9, -0.3], mats.dark);
	addBox(group, "monument_aqua_face", [13.5, 4.2, 0.28], [0, 4.1, -0.82], mats.aqua);
	addBox(group, "monument_coral_bar", [10.5, 0.42, 0.34], [0, 5.25, -1.05], mats.coral);
	for (let i = 0; i < 7; i += 1) {
		addBox(group, `monument_lane_notch_${i}`, [0.55, 0.22, 0.38], [-5.4 + i * 1.8, 3.35, -1.12], mats.whiteGlow);
	}
	for (let i = 0; i < 4; i += 1) {
		addCylinder(group, `monument_palm_trunk_${i}`, 0.18, 4.2, [-11 + i * 7.4, 2.7, 3.5], mats.wood);
		addSphere(group, `monument_palm_crown_${i}`, 1.35, [-11 + i * 7.4, 5.1, 3.5], mats.leaf);
	}
}

function buildLaunchCoastBeacon(root, mats) {
	const group = new THREE.Group();
	group.name = "launch_coast_beacon";
	group.position.set(44, 0, 32);
	group.rotation.y = -0.22;
	root.add(group);

	addCylinder(group, "beacon_round_pad", 8.0, 0.72, [0, 0.44, 0], mats.concrete);
	addCylinder(group, "beacon_core", 0.55, 13.0, [0, 7.0, 0], mats.steel);
	for (let i = 0; i < 4; i += 1) {
		const angle = (i / 4) * Math.PI * 2;
		addCylinder(
			group,
			`beacon_leg_${i}`,
			0.16,
			13.8,
			[Math.cos(angle) * 1.8, 6.9, Math.sin(angle) * 1.8],
			mats.bronze
		).rotation.z = 0.12 * (i % 2 === 0 ? 1 : -1);
	}
	for (let i = 0; i < 6; i += 1) {
		addTorus(group, `beacon_ring_${i}`, 2.05, 0.08, [0, 2.4 + i * 1.85, 0], mats.steel);
	}
	addSphere(group, "beacon_warning_light", 0.72, [0, 14.4, 0], mats.coral);
	for (let i = 0; i < 10; i += 1) {
		const angle = (i / 10) * Math.PI * 2;
		addBox(group, `beacon_pad_marker_${i}`, [1.2, 0.1, 0.34], [
			Math.cos(angle) * 5.9,
			0.86,
			Math.sin(angle) * 5.9
		], mats.amber).rotation.y = -angle;
	}
}

function buildGulfArtPier(root, mats) {
	const group = new THREE.Group();
	group.name = "gulf_art_pier";
	group.position.set(-52, 0, 28);
	group.rotation.y = -0.12;
	root.add(group);

	addBox(group, "pier_walkway", [48, 0.7, 7.5], [0, 0.45, 0], mats.wood);
	for (let i = 0; i < 13; i += 1) {
		addCylinder(group, `pier_piling_l_${i}`, 0.22, 3.5, [-23 + i * 4, -0.7, -3.1], mats.wood);
		addCylinder(group, `pier_piling_r_${i}`, 0.22, 3.5, [-23 + i * 4, -0.7, 3.1], mats.wood);
	}
	for (let i = 0; i < 6; i += 1) {
		const arch = addTorus(group, `pier_art_arc_${i}`, 2.1 + i * 0.36, 0.07, [-14 + i * 5.6, 3.0, 0], i % 2 === 0 ? mats.coral : mats.aqua);
		arch.rotation.x = Math.PI / 2;
		arch.rotation.z = Math.PI / 2;
	}
	for (let i = 0; i < 8; i += 1) {
		addCylinder(group, `pier_lamp_${i}`, 0.13, 2.6, [-20 + i * 5.7, 2.0, -4.1], mats.steel);
		addSphere(group, `pier_lamp_globe_${i}`, 0.28, [-20 + i * 5.7, 3.45, -4.1], mats.whiteGlow);
	}
}

function buildMicroLighting(root, mats) {
	const group = new THREE.Group();
	group.name = "shared_vista_micro_lights";
	root.add(group);
	for (let i = 0; i < 18; i += 1) {
		const x = -42 + (i % 9) * 10.5;
		const z = 48 + Math.floor(i / 9) * 5.4;
		addCylinder(group, `low_path_light_post_${i}`, 0.11, 1.1, [x, 0.9, z], mats.dark);
		addSphere(group, `low_path_light_globe_${i}`, 0.18, [x, 1.52, z], mats.amber);
	}
}

function addSeaGrape(parent, mats, x, y, z) {
	addCylinder(parent, `sea_grape_trunk_${x}_${z}`, 0.22, 1.8, [x, y + 0.9, z], mats.wood);
	addSphere(parent, `sea_grape_leaf_${x}_${z}_a`, 1.1, [x - 0.45, y + 2.1, z], mats.leaf);
	addSphere(parent, `sea_grape_leaf_${x}_${z}_b`, 1.0, [x + 0.55, y + 2.25, z + 0.3], mats.leaf);
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
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 18, 12), mat);
	return place(parent, mesh, name, position);
}

function addTorus(parent, name, radius, tube, position, mat) {
	const mesh = new THREE.Mesh(new THREE.TorusGeometry(radius, tube, 12, 48), mat);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
