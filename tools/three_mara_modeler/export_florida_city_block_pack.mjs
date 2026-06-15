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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_city_block_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_city_block_pack";

const materials = {
	limestone: material("warm_limestone_facade", 0xd7c6a8, 0.7, 0.0, 0.0, null),
	whiteStucco: material("salt_white_stucco", 0xf0eadf, 0.66, 0.0, 0.0, null),
	coralStucco: material("sunset_coral_stucco", 0xd96b5b, 0.62, 0.0, 0.0, null),
	blueGlass: material("blue_green_glass", 0x5bb8c7, 0.16, 0.0, 0.35, null),
	blackGlass: material("black_water_glass", 0x0d1218, 0.18, 0.0, 0.42, null),
	brass: material("brushed_brass_trim", 0xc79b48, 0.34, 0.72, 0.0, null),
	concrete: material("weathered_sidewalk_concrete", 0xa9a497, 0.88, 0.0, 0.0, null),
	asphalt: material("sealed_urban_asphalt", 0x20242a, 0.82, 0.0, 0.0, null),
	neonCyan: material("cyan_storefront_neon", 0x85f6ff, 0.28, 0.0, 0.0, 0x43f1ff),
	neonPink: material("pink_hotel_neon", 0xff6b97, 0.3, 0.0, 0.0, 0xff4c8a),
	amber: material("amber_lobby_glow", 0xffbf65, 0.34, 0.0, 0.0, 0xff9b35),
	palmLeaf: material("royal_palm_leaf", 0x2d6b43, 0.78, 0.0, 0.0, null),
	palmTrunk: material("ringed_palm_trunk", 0x8b6643, 0.84, 0.0, 0.0, null)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaCityBlockPack";
pack.userData.provenance = "Original Three.js-authored Florida city block; no commercial-game source assets.";
scene.add(pack);

buildStreetPlate(pack, materials);
buildStackedGlassTower(pack, materials, [-32, 0, -6], 88, 0.12);
buildBalconyCondo(pack, materials, [6, 0, -5], 66, -0.08);
buildDecoRetailHotel(pack, materials, [34, 0, -2], 42, 0.2);
buildRooftopDetails(pack, materials);
buildStreetFurniture(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_city_block = true;
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

function material(name, color, roughness, metalness, clearcoat, emissive) {
	const mat = new THREE.MeshPhysicalMaterial({
		name,
		color,
		roughness,
		metalness,
		clearcoat,
		clearcoatRoughness: 0.42,
		emissive: emissive ?? 0x000000,
		emissiveIntensity: emissive === null ? 0 : 1.55
	});
	mat.userData.godot_hint = "Three.js-authored original Florida city material";
	return mat;
}

function buildStreetPlate(root, mats) {
	addBox(root, "city_block_sidewalk", [92, 0.65, 54], [0, 0.325, 0], mats.concrete);
	addBox(root, "city_block_asphalt_lane", [92, 0.72, 12], [0, 0.72, 20], mats.asphalt);
	for (let i = 0; i < 9; i += 1) {
		addBox(root, `crosswalk_stripe_${i}`, [4.4, 0.08, 9.4], [-42 + i * 10.5, 1.1, 20], mats.whiteStucco);
	}
	for (const x of [-44, 44]) {
		addBox(root, `curb_${x}`, [1.1, 0.8, 54], [x, 1.0, 0], mats.limestone);
	}
}

function buildStackedGlassTower(root, mats, position, height, yaw) {
	const tower = new THREE.Group();
	tower.name = "stacked_glass_tower";
	tower.position.set(...position);
	tower.rotation.y = yaw;
	root.add(tower);

	addBox(tower, "tower_lobby", [20, 11, 17], [0, 6.2, 0], mats.limestone);
	addBox(tower, "tower_core", [16, height, 14], [0, 11 + height * 0.5, 0], mats.blackGlass);
	addBox(tower, "tower_offset_slab", [14, height * 0.62, 12], [8, 20 + height * 0.31, 2], mats.blueGlass);
	addBox(tower, "tower_roof_crown", [19, 4, 17], [1, height + 14, 0], mats.neonCyan);

	for (let floor = 0; floor < 17; floor += 1) {
		const y = 17 + floor * 4.5;
		addBox(tower, `tower_balcony_front_${floor}`, [18.2, 0.36, 1.1], [0, y, -7.6], mats.brass);
		addBox(tower, `tower_window_line_${floor}`, [0.35, 2.6, 14.7], [-8.25, y, 0], mats.neonCyan);
	}
}

function buildBalconyCondo(root, mats, position, height, yaw) {
	const condo = new THREE.Group();
	condo.name = "terraced_balcony_condo";
	condo.position.set(...position);
	condo.rotation.y = yaw;
	root.add(condo);

	addBox(condo, "condo_base", [25, 9, 16], [0, 5, 0], mats.whiteStucco);
	addBox(condo, "condo_body", [22, height, 15], [0, 10 + height * 0.5, 0], mats.blueGlass);
	for (let floor = 0; floor < 13; floor += 1) {
		const y = 15 + floor * 4.2;
		addBox(condo, `condo_balcony_front_${floor}`, [24, 0.42, 2.0], [0, y, -8.5], mats.whiteStucco);
		addBox(condo, `condo_balcony_rail_${floor}`, [24.4, 1.0, 0.34], [0, y + 0.75, -9.35], mats.brass);
		if (floor % 3 === 0) {
			addBox(condo, `condo_planter_${floor}`, [5.0, 0.9, 1.0], [-8, y + 0.55, -9.9], mats.palmLeaf);
		}
	}
	addBox(condo, "condo_pool_deck", [28, 0.9, 18], [0, 72, 0], mats.concrete);
	addBox(condo, "condo_rooftop_pool", [14, 0.35, 8], [0, 72.7, -1], mats.neonCyan);
}

function buildDecoRetailHotel(root, mats, position, height, yaw) {
	const hotel = new THREE.Group();
	hotel.name = "deco_retail_hotel";
	hotel.position.set(...position);
	hotel.rotation.y = yaw;
	root.add(hotel);

	addBox(hotel, "retail_podium", [30, 8, 18], [0, 4.5, 0], mats.coralStucco);
	addBox(hotel, "hotel_body", [24, height, 15], [0, 9 + height * 0.5, 0], mats.whiteStucco);
	addBox(hotel, "hotel_crown", [19, 7, 12], [0, height + 12, 0], mats.coralStucco);
	addBox(hotel, "hotel_vertical_neon_left", [0.55, height - 4, 0.6], [-12.4, 14 + (height - 4) * 0.5, -7.8], mats.neonPink);
	addBox(hotel, "hotel_vertical_neon_right", [0.55, height - 4, 0.6], [12.4, 14 + (height - 4) * 0.5, -7.8], mats.neonCyan);

	for (let i = 0; i < 6; i += 1) {
		addBox(hotel, `retail_awning_${i}`, [4.0, 0.7, 2.4], [-11 + i * 4.4, 8.8, -10], i % 2 === 0 ? mats.neonPink : mats.neonCyan);
	}
	for (let floor = 0; floor < 8; floor += 1) {
		for (let col = 0; col < 4; col += 1) {
			addBox(hotel, `hotel_window_${floor}_${col}`, [2.7, 2.6, 0.4], [-8.2 + col * 5.45, 15 + floor * 4.4, -7.75], mats.blueGlass);
		}
	}
}

function buildRooftopDetails(root, mats) {
	const details = new THREE.Group();
	details.name = "rooftop_mechanical_details";
	details.position.set(0, 0, 0);
	root.add(details);

	for (const entry of [
		[-32, 106, -6],
		[6, 78, -5],
		[34, 58, -2]
	]) {
		const [x, y, z] = entry;
		addBox(details, `hvac_box_${x}`, [5, 2.2, 4], [x - 3, y, z + 1], mats.concrete);
		addCylinder(details, `roof_vent_${x}`, 0.75, 3.2, [x + 4, y + 0.5, z - 3], mats.brass);
		addBox(details, `antenna_${x}`, [0.35, 10, 0.35], [x + 7, y + 5, z + 2], mats.neonCyan);
	}
}

function buildStreetFurniture(root, mats) {
	const items = new THREE.Group();
	items.name = "street_level_furniture";
	root.add(items);

	for (let i = 0; i < 7; i += 1) {
		const x = -38 + i * 12.5;
		addCylinder(items, `street_light_pole_${i}`, 0.18, 8, [x, 4.7, 16], mats.brass);
		addSphere(items, `street_light_globe_${i}`, 0.7, [x, 8.9, 16], mats.amber);
	}

	for (let i = 0; i < 8; i += 1) {
		const palm = new THREE.Group();
		palm.name = `royal_palm_${i}`;
		palm.position.set(-40 + i * 11.5, 1, -21);
		palm.rotation.y = i * 0.43;
		items.add(palm);
		addCylinder(palm, "palm_trunk", 0.42, 12, [0, 6, 0], mats.palmTrunk).rotation.z = 0.08;
		for (let j = 0; j < 8; j += 1) {
			const frond = addBox(palm, `palm_frond_${j}`, [0.7, 0.24, 7], [0, 12.8, 0], mats.palmLeaf);
			frond.rotation.y = (Math.PI * 2 * j) / 8;
			frond.rotation.x = 0.5;
			frond.translateZ(2.7);
		}
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
