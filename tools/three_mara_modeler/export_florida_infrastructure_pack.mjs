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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_infrastructure_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_infrastructure_pack";

const materials = {
	asphalt: material("sun_sealed_asphalt", 0x202328, 0.82, 0.0, 0.0, null, 0),
	concrete: material("pale_turnpike_concrete", 0xbfb9aa, 0.84, 0.0, 0.0, null, 0),
	paintWhite: material("road_paint_white", 0xf4f0df, 0.64, 0.0, 0.0, null, 0),
	paintYellow: material("road_paint_yellow", 0xf0c24a, 0.58, 0.0, 0.0, null, 0),
	steel: material("roadside_galvanized_steel", 0x757a7d, 0.4, 0.55, 0.0, null, 0),
	signGreen: material("reflective_route_green", 0x1e6f50, 0.36, 0.0, 0.15, null, 0),
	signBlue: material("reflective_services_blue", 0x245f9f, 0.34, 0.0, 0.14, null, 0),
	coral: material("coral_service_accent", 0xe06a55, 0.5, 0.0, 0.0, null, 0),
	aqua: material("aqua_service_accent", 0x42c8c6, 0.42, 0.0, 0.0, 0x20dce0, 0.65),
	warmLight: material("warm_service_light", 0xffc66f, 0.34, 0.0, 0.0, 0xff9d32, 1.7),
	wood: material("salt_weathered_wood", 0x8b7151, 0.9, 0.0, 0.0, null, 0),
	palmLeaf: material("roadside_palm_leaf", 0x2d6a3d, 0.78, 0.0, 0.0, null, 0),
	palmTrunk: material("ringed_palm_trunk", 0x8a6242, 0.86, 0.0, 0.0, null, 0),
	water: material("airboat_channel_water", 0x39aeb4, 0.28, 0.0, 0.3, 0x20d2dc, 0.45),
	boatWhite: material("airboat_white_fiberglass", 0xf1eadf, 0.52, 0.0, 0.12, null, 0),
	lifeguardRed: material("lifeguard_red", 0xd74b3f, 0.5, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaInfrastructurePack";
pack.userData.provenance = "Original Three.js-authored Florida road/beach/wetland infrastructure prop pack; no commercial-game source assets.";
scene.add(pack);

buildTurnpikeToll(pack, materials);
buildFuelStop(pack, materials);
buildBeachAccess(pack, materials);
buildAirboatDock(pack, materials);
buildRouteGantry(pack, materials);
buildUtilityAndPalms(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_infrastructure = true;
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
	mat.userData.godot_hint = "Three.js-authored original Florida infrastructure material";
	return mat;
}

function buildTurnpikeToll(root, mats) {
	const toll = new THREE.Group();
	toll.name = "turnpike_toll_plaza";
	toll.position.set(-58, 0, -16);
	root.add(toll);

	addBox(toll, "toll_road_plate", [70, 0.45, 30], [0, 0.32, 0], mats.asphalt);
	for (let lane = 0; lane < 5; lane += 1) {
		const x = -24 + lane * 12;
		addBox(toll, `lane_divider_${lane}`, [0.32, 0.08, 28], [x + 5.8, 0.62, 0], mats.paintWhite);
		addBox(toll, `toll_booth_${lane}`, [5.0, 5.2, 4.2], [x, 3.0, -2], lane % 2 === 0 ? mats.concrete : mats.signBlue);
		addBox(toll, `toll_window_${lane}`, [3.4, 2.0, 0.35], [x, 3.6, -4.25], mats.aqua);
		addBox(toll, `lane_light_${lane}`, [3.5, 0.45, 0.45], [x, 6.3, -4.5], lane % 2 === 0 ? mats.warmLight : mats.aqua);
	}
	for (const x of [-31, 31]) {
		addBox(toll, `toll_gantry_leg_${x}`, [1.0, 14, 1.0], [x, 7.2, -8], mats.steel);
	}
	addBox(toll, "toll_gantry_beam", [66, 1.2, 1.2], [0, 14.3, -8], mats.steel);
	addBox(toll, "toll_sign_panel", [28, 5, 0.8], [0, 17.5, -8.7], mats.signGreen);
	addBox(toll, "toll_sign_glow", [22, 0.5, 0.9], [0, 18.7, -9.25], mats.aqua);
}

function buildFuelStop(root, mats) {
	const stop = new THREE.Group();
	stop.name = "coastal_fuel_stop";
	stop.position.set(34, 0, -22);
	stop.rotation.y = -0.18;
	root.add(stop);

	addBox(stop, "fuel_lot", [46, 0.5, 32], [0, 0.3, 0], mats.concrete);
	addBox(stop, "market_body", [24, 8, 11], [8, 4.4, 8], mats.concrete);
	addBox(stop, "market_glass", [19, 4.0, 0.45], [8, 4.5, 2.3], mats.aqua);
	addBox(stop, "market_roof_band", [27, 1.2, 12], [8, 8.9, 8], mats.coral);
	addBox(stop, "pump_canopy", [36, 1.2, 14], [-4, 8.0, -8], mats.paintWhite);
	addBox(stop, "canopy_light_strip", [32, 0.35, 0.5], [-4, 7.25, -14.8], mats.warmLight);
	for (let i = 0; i < 4; i += 1) {
		const x = -17 + i * 8.5;
		addBox(stop, `fuel_pump_${i}`, [2.5, 3.7, 1.8], [x, 2.4, -8], i % 2 === 0 ? mats.signBlue : mats.coral);
		addBox(stop, `pump_screen_${i}`, [1.5, 0.9, 0.25], [x, 3.0, -8.95], mats.aqua);
	}
	for (const x of [-19, 11]) {
		addCylinder(stop, `canopy_post_${x}`, 0.35, 7.2, [x, 4.0, -13], mats.steel);
		addCylinder(stop, `canopy_post_back_${x}`, 0.35, 7.2, [x, 4.0, -3], mats.steel);
	}
}

function buildBeachAccess(root, mats) {
	const beach = new THREE.Group();
	beach.name = "beach_access_lifeguard";
	beach.position.set(50, 0, 24);
	beach.rotation.y = 0.3;
	root.add(beach);

	addBox(beach, "dune_walkover", [8, 0.65, 42], [0, 1.0, 4], mats.wood);
	addBox(beach, "lifeguard_platform", [13, 1.0, 10], [-14, 3.8, -4], mats.wood);
	for (const x of [-19, -9]) {
		for (const z of [-8, 0]) {
			addCylinder(beach, `lifeguard_post_${x}_${z}`, 0.25, 6.8, [x, 3.6, z], mats.wood);
		}
	}
	addBox(beach, "lifeguard_house", [11, 7, 8], [-14, 8.0, -4], mats.paintWhite);
	addBox(beach, "lifeguard_roof", [13, 1.4, 10], [-14, 12.0, -4], mats.lifeguardRed);
	addBox(beach, "lifeguard_window", [7, 3, 0.35], [-14, 8.6, -8.25], mats.aqua);
	addBox(beach, "beach_warning_panel", [4.4, 2.2, 0.35], [9, 3.0, -13], mats.lifeguardRed);
	for (let i = 0; i < 6; i += 1) {
		addBox(beach, `dune_rail_${i}`, [0.35, 1.4, 6], [-4.6 + (i % 2) * 9.2, 2.1, -15 + i * 7], mats.wood);
	}
}

function buildAirboatDock(root, mats) {
	const dock = new THREE.Group();
	dock.name = "wetland_airboat_dock";
	dock.position.set(-8, 0, 42);
	dock.rotation.y = -0.48;
	root.add(dock);

	addBox(dock, "airboat_channel", [46, 0.18, 24], [0, 0.3, 0], mats.water);
	addBox(dock, "airboat_dock_walk", [36, 0.75, 5], [0, 1.0, -6], mats.wood);
	addBox(dock, "airboat_finger", [5, 0.7, 22], [-10, 1.05, 6], mats.wood);
	for (let i = 0; i < 3; i += 1) {
		const x = -12 + i * 12;
		buildAirboat(dock, mats, x, 1.6, 5 + i * 2, i * 0.2);
	}
	for (let i = 0; i < 10; i += 1) {
		const x = -20 + (i % 5) * 10;
		const z = -12 + Math.floor(i / 5) * 22;
		addCylinder(dock, `reed_cluster_${i}`, 0.18, 4.2, [x, 2.2, z], mats.palmLeaf);
	}
}

function buildAirboat(parent, mats, x, y, z, yaw) {
	const boat = new THREE.Group();
	boat.name = "airboat";
	boat.position.set(x, y, z);
	boat.rotation.y = yaw;
	parent.add(boat);
	addBox(boat, "airboat_hull", [7.2, 1.2, 3.2], [0, 0, 0], mats.boatWhite);
	addBox(boat, "airboat_seat", [2.6, 2.4, 2.2], [-1.4, 1.4, 0], mats.steel);
	addCylinder(boat, "airboat_fan_ring", 2.0, 0.35, [3.0, 2.1, 0], mats.steel).rotation.z = Math.PI / 2;
	addBox(boat, "airboat_prop", [0.3, 4.0, 0.22], [3.0, 2.1, 0], mats.steel);
	addBox(boat, "airboat_prop_cross", [0.3, 0.22, 4.0], [3.0, 2.1, 0], mats.steel);
}

function buildRouteGantry(root, mats) {
	const gantry = new THREE.Group();
	gantry.name = "state_route_gantry";
	gantry.position.set(-2, 0, -50);
	root.add(gantry);

	addBox(gantry, "gantry_road", [72, 0.45, 18], [0, 0.32, 0], mats.asphalt);
	for (const x of [-28, 28]) {
		addBox(gantry, `route_gantry_leg_${x}`, [1.0, 16, 1.0], [x, 8.2, -3], mats.steel);
	}
	addBox(gantry, "route_gantry_beam", [60, 1.1, 1.1], [0, 16.5, -3], mats.steel);
	for (let i = 0; i < 3; i += 1) {
		addBox(gantry, `route_panel_${i}`, [15, 6, 0.6], [-19 + i * 19, 13.0, -3.8], i === 1 ? mats.signBlue : mats.signGreen);
		addBox(gantry, `route_panel_line_${i}`, [10, 0.45, 0.7], [-19 + i * 19, 14.1, -4.2], mats.paintWhite);
		addBox(gantry, `route_panel_arrow_${i}`, [3.8, 0.45, 0.7], [-19 + i * 19, 11.5, -4.2], mats.paintYellow);
	}
	for (let i = 0; i < 8; i += 1) {
		addBox(gantry, `lane_dash_${i}`, [4.0, 0.08, 0.45], [-32 + i * 9, 0.62, 0], mats.paintWhite);
	}
}

function buildUtilityAndPalms(root, mats) {
	const utility = new THREE.Group();
	utility.name = "utility_palms_and_service_details";
	utility.position.set(0, 0, 0);
	root.add(utility);

	for (let i = 0; i < 8; i += 1) {
		const x = -44 + i * 12;
		addCylinder(utility, `utility_pole_${i}`, 0.32, 12, [x, 6.2, 63], mats.wood);
		addBox(utility, `utility_crossarm_${i}`, [6.2, 0.35, 0.35], [x, 11.2, 63], mats.wood);
		addBox(utility, `utility_wire_${i}`, [10.0, 0.08, 0.08], [x + 5.8, 11.3, 63], mats.steel);
	}
	for (let i = 0; i < 7; i += 1) {
		const palm = new THREE.Group();
		palm.name = `infrastructure_palm_${i}`;
		palm.position.set(-39 + i * 13, 0.6, -68);
		palm.rotation.y = i * 0.55;
		utility.add(palm);
		addCylinder(palm, "palm_trunk", 0.45, 12.5, [0, 6.4, 0], mats.palmTrunk).rotation.z = 0.09;
		for (let j = 0; j < 8; j += 1) {
			const frond = addBox(palm, `palm_frond_${j}`, [0.72, 0.25, 7.4], [0, 13.1, 0], mats.palmLeaf);
			frond.rotation.y = (Math.PI * 2 * j) / 8;
			frond.rotation.x = 0.48;
			frond.translateZ(2.8);
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

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
