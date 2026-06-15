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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_environment_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_environment_pack";

const materials = {
	sand: material("wind_rippled_sand", 0xd9c287, 0.88, 0.0, 0.0, null, 0),
	duneGrass: material("sea_oat_dune_grass", 0x8f9a54, 0.9, 0.0, 0.0, null, 0),
	mangroveLeaf: material("mangrove_leaf_cluster", 0x234a32, 0.86, 0.0, 0.0, null, 0),
	mangroveRoot: material("mangrove_root_wood", 0x6f4f35, 0.92, 0.0, 0.0, null, 0),
	concrete: material("shell_path_concrete", 0xc4bdad, 0.82, 0.0, 0.0, null, 0),
	paintedWood: material("painted_kiosk_wood", 0xefe3cb, 0.66, 0.0, 0.0, null, 0),
	coral: material("coral_wayfinding_paint", 0xd95e52, 0.58, 0.0, 0.0, null, 0),
	aqua: material("aqua_wayfinding_paint", 0x4cced0, 0.5, 0.0, 0.0, 0x29dce0, 0.45),
	steel: material("powder_coated_street_steel", 0x52595d, 0.48, 0.35, 0.0, null, 0),
	rubber: material("black_recycled_rubber", 0x17191b, 0.8, 0.0, 0.0, null, 0),
	amber: material("small_warm_marker_light", 0xffbf6b, 0.34, 0.0, 0.0, 0xff9b32, 1.3),
	green: material("planter_leaf_mix", 0x2f7044, 0.84, 0.0, 0.0, null, 0),
	plasticBlue: material("service_bin_blue", 0x276aa0, 0.7, 0.0, 0.0, null, 0),
	plasticYellow: material("traffic_cone_yellow", 0xe6b83f, 0.68, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaEnvironmentPack";
pack.userData.provenance = "Original Three.js-authored Florida environmental detail prop pack; no commercial-game source assets.";
scene.add(pack);

buildBeachDunes(pack, materials);
buildMangrovePatch(pack, materials);
buildRoadsidePlanters(pack, materials);
buildMarketKiosks(pack, materials);
buildStreetFurniture(pack, materials);
buildTrailWayfinding(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_environment = true;
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
	mat.userData.godot_hint = "Three.js-authored original Florida environment material";
	return mat;
}

function buildBeachDunes(root, mats) {
	const dunes = new THREE.Group();
	dunes.name = "beach_dunes_and_sea_oats";
	dunes.position.set(-52, 0, -28);
	root.add(dunes);

	for (let i = 0; i < 7; i += 1) {
		const mound = addCylinder(dunes, `sand_dune_mound_${i}`, 3.2 + (i % 3) * 0.8, 1.1, [-27 + i * 9, 0.55, Math.sin(i) * 3], mats.sand);
		mound.scale.z = 0.45;
		mound.rotation.y = i * 0.37;
	}
	for (let i = 0; i < 34; i += 1) {
		const x = -31 + (i % 17) * 3.9;
		const z = -6 + Math.floor(i / 17) * 8 + Math.sin(i * 0.8) * 1.2;
		const grass = addBox(dunes, `sea_oat_blade_${i}`, [0.28, 3.5 + (i % 4) * 0.45, 0.2], [x, 2.0, z], mats.duneGrass);
		grass.rotation.z = -0.22 + (i % 5) * 0.08;
	}
	addBox(dunes, "beach_access_mat", [44, 0.22, 4.5], [0, 0.72, 9], mats.concrete);
}

function buildMangrovePatch(root, mats) {
	const patch = new THREE.Group();
	patch.name = "mangrove_edge_patch";
	patch.position.set(42, 0, -18);
	patch.rotation.y = -0.2;
	root.add(patch);

	for (let i = 0; i < 18; i += 1) {
		const x = -24 + (i % 6) * 9.2;
		const z = -10 + Math.floor(i / 6) * 8.5;
		addCylinder(patch, `mangrove_root_a_${i}`, 0.18, 4.8, [x - 0.8, 2.4, z], mats.mangroveRoot).rotation.z = 0.22;
		addCylinder(patch, `mangrove_root_b_${i}`, 0.16, 4.4, [x + 0.7, 2.2, z + 0.5], mats.mangroveRoot).rotation.z = -0.18;
		addSphere(patch, `mangrove_leaf_${i}`, 2.4 + (i % 3) * 0.35, [x, 5.7, z], mats.mangroveLeaf);
	}
	addBox(patch, "wetland_marker_board", [8, 3.2, 0.45], [24, 4.0, -6], mats.aqua);
	addCylinder(patch, "wetland_marker_post_l", 0.18, 4.2, [21, 2.1, -6], mats.steel);
	addCylinder(patch, "wetland_marker_post_r", 0.18, 4.2, [27, 2.1, -6], mats.steel);
}

function buildRoadsidePlanters(root, mats) {
	const planters = new THREE.Group();
	planters.name = "roadside_planters";
	planters.position.set(0, 0, -52);
	root.add(planters);

	for (let i = 0; i < 10; i += 1) {
		const x = -42 + i * 9.4;
		addBox(planters, `planter_box_${i}`, [6.0, 1.2, 2.4], [x, 0.95, 0], mats.concrete);
		for (let j = 0; j < 4; j += 1) {
			addSphere(planters, `planter_leaf_${i}_${j}`, 0.9, [x - 2.1 + j * 1.4, 2.0, 0], mats.green);
		}
		if (i % 2 === 0) {
			addBox(planters, `planter_light_${i}`, [3.8, 0.25, 0.28], [x, 2.15, -1.25], mats.amber);
		}
	}
}

function buildMarketKiosks(root, mats) {
	const kiosks = new THREE.Group();
	kiosks.name = "coastal_market_kiosks";
	kiosks.position.set(26, 0, 26);
	kiosks.rotation.y = 0.28;
	root.add(kiosks);

	for (let i = 0; i < 4; i += 1) {
		const x = -18 + i * 12;
		addBox(kiosks, `kiosk_body_${i}`, [8, 5.2, 6], [x, 2.8, 0], mats.paintedWood);
		addBox(kiosks, `kiosk_roof_${i}`, [9.5, 1.0, 7.5], [x, 5.9, 0], i % 2 === 0 ? mats.coral : mats.aqua);
		addBox(kiosks, `kiosk_counter_${i}`, [6.8, 1.0, 1.2], [x, 2.2, -3.6], mats.steel);
		addBox(kiosks, `kiosk_glow_${i}`, [5.4, 0.28, 0.35], [x, 4.5, -3.85], mats.amber);
	}
	addBox(kiosks, "market_shade_sail", [54, 0.35, 10], [0, 8.2, 2], mats.aqua);
}

function buildStreetFurniture(root, mats) {
	const furniture = new THREE.Group();
	furniture.name = "street_furniture_and_service_props";
	furniture.position.set(-20, 0, 38);
	root.add(furniture);

	for (let i = 0; i < 8; i += 1) {
		const x = -30 + i * 8.5;
		addBox(furniture, `bench_seat_${i}`, [5.5, 0.35, 1.4], [x, 1.2, 0], mats.wood ?? mats.steel);
		addBox(furniture, `bench_back_${i}`, [5.5, 1.6, 0.35], [x, 2.0, 0.75], mats.steel);
		if (i % 2 === 0) {
			addBox(furniture, `service_bin_${i}`, [1.6, 2.4, 1.6], [x + 4.2, 1.3, -0.2], mats.plasticBlue);
		}
	}
	for (let i = 0; i < 12; i += 1) {
		const cone = addCylinder(furniture, `traffic_cone_${i}`, 0.45, 1.4, [-34 + i * 6.2, 0.95, -6], mats.plasticYellow);
		cone.scale.x = 0.7;
		cone.scale.z = 0.7;
		addBox(furniture, `cone_band_${i}`, [0.95, 0.18, 0.95], [-34 + i * 6.2, 1.35, -6], mats.paintWhite ?? mats.concrete);
	}
}

function buildTrailWayfinding(root, mats) {
	const signs = new THREE.Group();
	signs.name = "trail_wayfinding_signs";
	signs.position.set(52, 0, 36);
	signs.rotation.y = -0.42;
	root.add(signs);

	for (let i = 0; i < 6; i += 1) {
		const x = -18 + i * 7.2;
		addCylinder(signs, `wayfinding_post_${i}`, 0.16, 4.4, [x, 2.2, 0], mats.steel);
		addBox(signs, `wayfinding_panel_${i}`, [5.4, 2.1, 0.34], [x, 4.4, 0], i % 2 === 0 ? mats.signGreen ?? mats.aqua : mats.coral);
		addBox(signs, `wayfinding_stripe_${i}`, [3.4, 0.25, 0.4], [x, 4.75, -0.22], mats.paintWhite ?? mats.concrete);
	}
}

function addBox(parent, name, size, position, mat) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), mat);
	return place(parent, mesh, name, position);
}

function addCylinder(parent, name, radius, height, position, mat) {
	const mesh = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, height, 16), mat);
	return place(parent, mesh, name, position);
}

function addSphere(parent, name, radius, position, mat) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 18, 12), mat);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
