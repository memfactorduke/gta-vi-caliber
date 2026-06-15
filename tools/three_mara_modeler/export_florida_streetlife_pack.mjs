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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_streetlife_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_streetlife_pack";

const materials = {
	pavement: material("warm_salt_pavement", 0xbdb4a2, 0.86, 0.0, 0.0, null, 0),
	wetAsphalt: material("wet_night_asphalt", 0x14181f, 0.22, 0.0, 0.34, null, 0),
	stuccoWhite: material("soft_white_stucco", 0xe9dfcc, 0.72, 0.0, 0.05, null, 0),
	stuccoPink: material("rose_stucco", 0xd98886, 0.68, 0.0, 0.05, null, 0),
	tileBlue: material("blue_ceramic_tile", 0x3f89b5, 0.35, 0.0, 0.2, null, 0),
	darkGlass: material("deep_reflective_glass", 0x172331, 0.16, 0.0, 0.42, null, 0),
	tealGlow: material("teal_window_glow", 0x42d8d2, 0.28, 0.0, 0.0, 0x20e0d8, 1.6),
	coralGlow: material("coral_neon_glow", 0xff5d57, 0.3, 0.0, 0.0, 0xff4a42, 1.8),
	amberGlow: material("amber_cafe_glow", 0xffbd72, 0.35, 0.0, 0.0, 0xff9a38, 1.4),
	creamGlow: material("cream_interior_glow", 0xffefd0, 0.4, 0.0, 0.0, 0xffefd0, 1.2),
	metal: material("powder_coated_black_metal", 0x16191b, 0.5, 0.62, 0.04, null, 0),
	brass: material("brushed_brass_detail", 0xc29a52, 0.34, 0.7, 0.08, null, 0),
	canvas: material("striped_canvas_awning", 0x2aa4a8, 0.7, 0.0, 0.0, null, 0),
	coralCanvas: material("coral_canvas_awning", 0xd55e55, 0.72, 0.0, 0.0, null, 0),
	wood: material("dark_teak_furniture", 0x65472e, 0.72, 0.0, 0.0, null, 0),
	leaf: material("tropical_planter_leaf", 0x2f7144, 0.78, 0.0, 0.0, null, 0),
	clay: material("terracotta_planter", 0xb56043, 0.78, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaStreetlifePack";
pack.userData.provenance = "Original Three.js-authored Florida streetlife and nightlife pack; no commercial-game source assets.";
scene.add(pack);

buildBoutiqueHotelFront(pack, materials);
buildOutdoorDiningBlock(pack, materials);
buildRooftopLounge(pack, materials);
buildNightMarketStalls(pack, materials);
buildClubEntry(pack, materials);
buildMicroStreetDetails(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_streetlife = true;
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
		clearcoatRoughness: 0.38,
		emissive: emissive ?? 0x000000,
		emissiveIntensity
	});
	mat.userData.godot_hint = "Three.js-authored original Florida streetlife material";
	return mat;
}

function buildBoutiqueHotelFront(root, mats) {
	const group = new THREE.Group();
	group.name = "boutique_hotel_front";
	group.position.set(-46, 0, -26);
	root.add(group);

	addBox(group, "hotel_sidewalk", [46, 0.45, 18], [0, 0.28, 0], mats.pavement);
	addBox(group, "hotel_facade_low", [36, 7.0, 3.2], [0, 3.7, -6.8], mats.stuccoWhite);
	addBox(group, "hotel_facade_high", [18, 11.0, 3.0], [-9, 5.7, -7.2], mats.stuccoPink);
	for (let i = 0; i < 12; i += 1) {
		const x = -16 + (i % 6) * 6.4;
		const y = 2.8 + Math.floor(i / 6) * 3.4;
		addBox(group, `hotel_lit_window_${i}`, [2.5, 1.45, 0.16], [x, y, -8.5], i % 3 === 0 ? mats.tealGlow : mats.creamGlow);
		addBox(group, `hotel_window_frame_${i}`, [2.9, 1.75, 0.08], [x, y, -8.58], mats.metal);
	}
	for (let i = 0; i < 5; i += 1) {
		addBox(group, `hotel_awning_${i}`, [5.2, 0.35, 3.8], [-14 + i * 7, 2.5, -4.6], i % 2 === 0 ? mats.canvas : mats.coralCanvas);
		addCylinder(group, `hotel_awning_rod_${i}`, 0.07, 5.4, [-14 + i * 7, 2.32, -2.8], mats.brass).rotation.z = Math.PI / 2;
	}
	addBox(group, "hotel_neon_blade", [0.7, 5.8, 0.32], [19.3, 5.4, -5.2], mats.coralGlow);
	for (let i = 0; i < 8; i += 1) {
		addPlanter(group, mats, -19 + i * 5.4, 0.55, 3.9);
	}
}

function buildOutdoorDiningBlock(root, mats) {
	const group = new THREE.Group();
	group.name = "outdoor_dining_block";
	group.position.set(18, 0, -32);
	group.rotation.y = 0.16;
	root.add(group);

	addBox(group, "dining_patio", [44, 0.42, 22], [0, 0.25, 0], mats.pavement);
	for (let i = 0; i < 10; i += 1) {
		const x = -18 + (i % 5) * 9;
		const z = -5 + Math.floor(i / 5) * 9.5;
		addCylinder(group, `cafe_table_${i}`, 1.0, 0.2, [x, 1.05, z], mats.wood);
		addCylinder(group, `cafe_table_stem_${i}`, 0.13, 1.0, [x, 0.62, z], mats.metal);
		for (let c = 0; c < 4; c += 1) {
			const angle = (c / 4) * Math.PI * 2;
			addBox(group, `cafe_chair_${i}_${c}`, [0.9, 0.18, 0.8], [
				x + Math.cos(angle) * 1.75,
				0.86,
				z + Math.sin(angle) * 1.75
			], mats.wood).rotation.y = -angle;
		}
		addSphere(group, `cafe_table_light_${i}`, 0.22, [x, 1.38, z], mats.amberGlow);
	}
	for (let i = 0; i < 6; i += 1) {
		addUmbrella(group, mats, -18 + i * 7.2, 0.65, -11.0, i % 2 === 0 ? mats.canvas : mats.coralCanvas);
	}
	for (let i = 0; i < 7; i += 1) {
		addBox(group, `patio_string_wire_${i}`, [0.08, 0.08, 18], [-21 + i * 7, 4.2, 0], mats.metal);
		addSphere(group, `patio_string_bulb_${i}`, 0.2, [-21 + i * 7, 3.9, 0], mats.creamGlow);
	}
}

function buildRooftopLounge(root, mats) {
	const group = new THREE.Group();
	group.name = "rooftop_lounge_module";
	group.position.set(46, 0, 12);
	group.rotation.y = -0.24;
	root.add(group);

	addBox(group, "lounge_building_mass", [34, 9.0, 24], [0, 4.5, 0], mats.stuccoWhite);
	addBox(group, "lounge_roof_deck", [36, 0.55, 26], [0, 9.35, 0], mats.pavement);
	addBox(group, "lounge_pool", [17, 0.16, 6], [-6, 9.72, -5], mats.tealGlow);
	addBox(group, "lounge_bar", [12, 1.2, 3.2], [9, 10.15, 7.2], mats.wood);
	addBox(group, "lounge_bar_backlight", [10.8, 0.24, 0.16], [9, 11.0, 5.46], mats.coralGlow);
	for (let i = 0; i < 12; i += 1) {
		addBox(group, `roof_glass_rail_${i}`, [2.4, 1.2, 0.12], [-16 + i * 2.9, 10.3, -12.8], mats.darkGlass);
	}
	for (let i = 0; i < 8; i += 1) {
		addBox(group, `lounge_daybed_${i}`, [3.0, 0.35, 1.4], [-15 + i * 4.4, 9.82, 2.0], mats.coralCanvas);
		addBox(group, `lounge_daybed_frame_${i}`, [3.3, 0.18, 1.7], [-15 + i * 4.4, 9.6, 2.0], mats.metal);
	}
	for (let i = 0; i < 8; i += 1) {
		addCylinder(group, `roof_marker_light_${i}`, 0.16, 0.18, [-15 + i * 4.4, 9.85, 11.2], mats.amberGlow);
	}
}

function buildNightMarketStalls(root, mats) {
	const group = new THREE.Group();
	group.name = "night_market_stalls";
	group.position.set(-18, 0, 30);
	group.rotation.y = 0.34;
	root.add(group);

	addBox(group, "market_walkway", [54, 0.42, 20], [0, 0.25, 0], mats.wetAsphalt);
	for (let i = 0; i < 8; i += 1) {
		const x = -23 + i * 6.6;
		addBox(group, `market_stall_body_${i}`, [4.6, 2.1, 3.2], [x, 1.3, -4.2], i % 2 === 0 ? mats.stuccoWhite : mats.stuccoPink);
		addBox(group, `market_stall_counter_${i}`, [4.2, 0.45, 0.7], [x, 1.2, -2.35], mats.wood);
		addBox(group, `market_stall_canopy_${i}`, [5.2, 0.35, 4.5], [x, 2.65, -4.2], i % 2 === 0 ? mats.canvas : mats.coralCanvas);
		addSphere(group, `market_stall_lantern_${i}`, 0.28, [x, 2.25, -2.4], i % 3 === 0 ? mats.tealGlow : mats.amberGlow);
	}
	for (let i = 0; i < 9; i += 1) {
		addCylinder(group, `market_bollard_${i}`, 0.18, 1.1, [-26 + i * 6.5, 0.9, 6.8], mats.brass);
	}
}

function buildClubEntry(root, mats) {
	const group = new THREE.Group();
	group.name = "premium_club_entry";
	group.position.set(28, 0, 34);
	group.rotation.y = -0.46;
	root.add(group);

	addBox(group, "club_entry_plaza", [34, 0.45, 17], [0, 0.28, 0], mats.pavement);
	addBox(group, "club_front_wall", [25, 6.8, 2.2], [0, 3.6, -6.2], mats.darkGlass);
	addBox(group, "club_door_glow", [4.8, 4.2, 0.24], [0, 2.8, -7.45], mats.tealGlow);
	addBox(group, "club_neon_header", [17, 0.52, 0.3], [0, 6.7, -7.58], mats.coralGlow);
	for (let i = 0; i < 7; i += 1) {
		addCylinder(group, `queue_post_l_${i}`, 0.13, 1.35, [-12 + i * 4, 1.05, 2.4], mats.metal);
		addCylinder(group, `queue_post_r_${i}`, 0.13, 1.35, [-12 + i * 4, 1.05, 5.0], mats.metal);
		addBox(group, `queue_rope_${i}`, [3.2, 0.12, 0.12], [-10 + i * 4, 1.65, 2.4], mats.coralGlow);
		addBox(group, `queue_rope_back_${i}`, [3.2, 0.12, 0.12], [-10 + i * 4, 1.65, 5.0], mats.tealGlow);
	}
	for (let i = 0; i < 5; i += 1) {
		addCylinder(group, `club_uplight_${i}`, 0.22, 0.2, [-10 + i * 5, 0.68, -3.7], mats.amberGlow);
	}
}

function buildMicroStreetDetails(root, mats) {
	const group = new THREE.Group();
	group.name = "shared_micro_street_details";
	root.add(group);

	for (let i = 0; i < 12; i += 1) {
		addPlanter(group, mats, -54 + i * 9.5, 0.45, 50);
	}
	for (let i = 0; i < 10; i += 1) {
		addBox(group, `metal_bench_${i}`, [2.5, 0.28, 0.7], [-46 + i * 10, 0.9, 56], mats.metal);
		addBox(group, `bench_back_${i}`, [2.5, 0.82, 0.16], [-46 + i * 10, 1.25, 56.45], mats.wood);
	}
	for (let i = 0; i < 14; i += 1) {
		addCylinder(group, `short_waylight_${i}`, 0.1, 1.25, [-58 + i * 8.9, 0.95, 62], mats.metal);
		addSphere(group, `short_waylight_globe_${i}`, 0.2, [-58 + i * 8.9, 1.65, 62], mats.amberGlow);
	}
}

function addUmbrella(parent, mats, x, y, z, canopyMat) {
	addCylinder(parent, `umbrella_pole_${x}`, 0.11, 3.0, [x, y + 1.5, z], mats.metal);
	addCylinder(parent, `umbrella_canopy_${x}`, 2.4, 0.35, [x, y + 3.0, z], canopyMat);
}

function addPlanter(parent, mats, x, y, z) {
	addBox(parent, `planter_box_${x}_${z}`, [2.1, 0.85, 1.2], [x, y, z], mats.clay);
	addSphere(parent, `planter_leaf_a_${x}_${z}`, 0.7, [x - 0.45, y + 0.85, z], mats.leaf);
	addSphere(parent, `planter_leaf_b_${x}_${z}`, 0.62, [x + 0.38, y + 0.92, z + 0.18], mats.leaf);
}

function addBox(parent, name, size, position, mat) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), mat);
	return place(parent, mesh, name, position);
}

function addCylinder(parent, name, radius, height, position, mat) {
	const mesh = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, height, 18), mat);
	return place(parent, mesh, name, position);
}

function addSphere(parent, name, radius, position, mat) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 16, 10), mat);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
