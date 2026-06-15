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
const outputPath = resolve(repoRoot, "game/assets/buildings/florida_traffic_marine_pack.glb");

const scene = new THREE.Scene();
scene.name = "florida_traffic_marine_pack";

const materials = {
	asphalt: material("traffic_asphalt_patch", 0x25282d, 0.84, 0.0, 0.0, null, 0),
	concrete: material("marina_concrete", 0xb8b1a3, 0.82, 0.0, 0.0, null, 0),
	carWhite: material("pearl_white_car_paint", 0xf0eee4, 0.34, 0.0, 0.35, null, 0),
	carBlue: material("aqua_blue_car_paint", 0x45b7ce, 0.32, 0.0, 0.32, null, 0),
	carCoral: material("coral_car_paint", 0xd85a4d, 0.36, 0.0, 0.28, null, 0),
	truckYellow: material("sun_yellow_service_truck", 0xd8ad3d, 0.54, 0.0, 0.08, null, 0),
	black: material("black_rubber_trim", 0x111315, 0.78, 0.0, 0.0, null, 0),
	glass: material("smoked_vehicle_glass", 0x243946, 0.2, 0.0, 0.35, null, 0),
	chrome: material("polished_vehicle_chrome", 0xaab5bb, 0.2, 0.76, 0.0, null, 0),
	boatWhite: material("white_marine_fiberglass", 0xf3eee2, 0.48, 0.0, 0.18, null, 0),
	boatTeal: material("teal_marine_accent", 0x36c2c7, 0.38, 0.0, 0.18, 0x1adbe0, 0.35),
	patrolNavy: material("navy_patrol_hull", 0x162d46, 0.52, 0.0, 0.12, null, 0),
	amber: material("amber_marker_light", 0xffb85d, 0.32, 0.0, 0.0, 0xff8e2c, 1.5),
	red: material("red_marker_light", 0xff4a3d, 0.35, 0.0, 0.0, 0xff2a22, 1.4),
	blueLight: material("blue_marker_light", 0x48a8ff, 0.3, 0.0, 0.0, 0x2a8fff, 1.4),
	buoyOrange: material("orange_channel_buoy", 0xe26a2c, 0.62, 0.0, 0.0, null, 0)
};

const pack = new THREE.Group();
pack.name = "ThreeJsFloridaTrafficMarinePack";
pack.userData.provenance = "Original Three.js-authored Florida traffic and marine set dressing pack; no commercial-game source assets.";
scene.add(pack);

buildRoadTraffic(pack, materials);
buildServiceVehicles(pack, materials);
buildMarineCluster(pack, materials);
buildHarborDetails(pack, materials);

pack.traverse((node) => {
	if (!node.isMesh) {
		return;
	}
	node.castShadow = true;
	node.receiveShadow = true;
	node.userData.threejs_florida_traffic_marine = true;
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
		clearcoatRoughness: 0.4,
		emissive: emissive ?? 0x000000,
		emissiveIntensity
	});
	mat.userData.godot_hint = "Three.js-authored original Florida traffic/marine material";
	return mat;
}

function buildRoadTraffic(root, mats) {
	const road = new THREE.Group();
	road.name = "parked_road_traffic";
	road.position.set(-36, 0, -20);
	root.add(road);

	addBox(road, "road_patch", [72, 0.42, 26], [0, 0.25, 0], mats.asphalt);
	for (let i = 0; i < 8; i += 1) {
		addBox(road, `lane_dash_${i}`, [4.4, 0.08, 0.5], [-30 + i * 8.8, 0.52, 0], mats.concrete);
	}
	const carMats = [mats.carWhite, mats.carBlue, mats.carCoral, mats.truckYellow];
	for (let i = 0; i < 7; i += 1) {
		const x = -28 + i * 9.6;
		const z = i % 2 === 0 ? -7 : 7;
		buildCar(road, mats, x, 0.9, z, i % 2 === 0 ? 0.05 : Math.PI + 0.04, carMats[i % carMats.length]);
	}
	for (let i = 0; i < 4; i += 1) {
		buildScooter(road, mats, -24 + i * 14, 0.8, 12, -0.25 + i * 0.12);
	}
}

function buildServiceVehicles(root, mats) {
	const service = new THREE.Group();
	service.name = "service_truck_and_trailer";
	service.position.set(32, 0, -28);
	service.rotation.y = -0.18;
	root.add(service);

	addBox(service, "service_lot", [42, 0.45, 24], [0, 0.28, 0], mats.concrete);
	buildBoxTruck(service, mats, -9, 1.2, 0, 0.08);
	addBox(service, "boat_trailer_frame", [15, 0.35, 4.2], [12, 1.0, 0], mats.chrome);
	addCylinder(service, "trailer_wheel_l", 0.8, 0.45, [7.0, 0.8, -2.3], mats.black).rotation.z = Math.PI / 2;
	addCylinder(service, "trailer_wheel_r", 0.8, 0.45, [7.0, 0.8, 2.3], mats.black).rotation.z = Math.PI / 2;
	buildSpeedboat(service, mats, 13, 2.0, 0, Math.PI / 2, mats.boatTeal);
}

function buildMarineCluster(root, mats) {
	const marina = new THREE.Group();
	marina.name = "marine_channel_traffic";
	marina.position.set(-18, 0, 32);
	marina.rotation.y = 0.28;
	root.add(marina);

	addBox(marina, "marina_water_plate", [60, 0.16, 34], [0, 0.2, 0], mats.boatTeal);
	for (let i = 0; i < 5; i += 1) {
		buildSpeedboat(marina, mats, -24 + i * 12, 1.1, -5 + (i % 2) * 11, 0.2 + i * 0.18, i % 2 === 0 ? mats.boatWhite : mats.carBlue);
	}
	for (let i = 0; i < 3; i += 1) {
		buildJetSki(marina, mats, -18 + i * 17, 0.85, 13, -0.1 + i * 0.2);
	}
	buildPatrolBoat(marina, mats, 20, 1.25, -13, -0.35);
}

function buildHarborDetails(root, mats) {
	const details = new THREE.Group();
	details.name = "harbor_buoys_and_bollards";
	details.position.set(34, 0, 28);
	root.add(details);

	addBox(details, "quay_strip", [48, 0.7, 8], [0, 0.65, 0], mats.concrete);
	for (let i = 0; i < 8; i += 1) {
		addCylinder(details, `dock_bollard_${i}`, 0.42, 1.2, [-21 + i * 6, 1.5, -2.7], mats.chrome);
	}
	for (let i = 0; i < 7; i += 1) {
		const buoy = addCylinder(details, `channel_buoy_${i}`, 0.8, 2.2, [-24 + i * 8, 1.3, 12], mats.buoyOrange);
		buoy.scale.y = 1.0 + (i % 2) * 0.2;
		addSphere(details, `buoy_light_${i}`, 0.35, [-24 + i * 8, 2.65, 12], i % 2 === 0 ? mats.red : mats.blueLight);
	}
}

function buildCar(parent, mats, x, y, z, yaw, paintMat) {
	const car = new THREE.Group();
	car.name = "parked_car";
	car.position.set(x, y, z);
	car.rotation.y = yaw;
	parent.add(car);
	addBox(car, "car_body", [5.6, 1.4, 2.7], [0, 0.7, 0], paintMat);
	addBox(car, "car_cabin", [3.2, 1.25, 2.2], [-0.4, 1.65, 0], mats.glass);
	addBox(car, "front_light_l", [0.25, 0.28, 0.55], [-2.95, 0.9, -0.78], mats.amber);
	addBox(car, "front_light_r", [0.25, 0.28, 0.55], [-2.95, 0.9, 0.78], mats.amber);
	for (const wx of [-1.9, 1.9]) {
		for (const wz of [-1.48, 1.48]) {
			addCylinder(car, `wheel_${wx}_${wz}`, 0.55, 0.42, [wx, 0.35, wz], mats.black).rotation.x = Math.PI / 2;
		}
	}
}

function buildScooter(parent, mats, x, y, z, yaw) {
	const scooter = new THREE.Group();
	scooter.name = "parked_scooter";
	scooter.position.set(x, y, z);
	scooter.rotation.y = yaw;
	parent.add(scooter);
	addBox(scooter, "scooter_deck", [2.2, 0.35, 0.65], [0, 0.55, 0], mats.carCoral);
	addBox(scooter, "scooter_seat", [1.0, 0.35, 0.62], [-0.1, 1.15, 0], mats.black);
	addCylinder(scooter, "front_wheel", 0.34, 0.18, [-0.95, 0.35, 0], mats.black).rotation.z = Math.PI / 2;
	addCylinder(scooter, "rear_wheel", 0.34, 0.18, [0.95, 0.35, 0], mats.black).rotation.z = Math.PI / 2;
	addBox(scooter, "handlebar", [0.22, 1.6, 0.22], [-0.95, 1.45, 0], mats.chrome);
}

function buildBoxTruck(parent, mats, x, y, z, yaw) {
	const truck = new THREE.Group();
	truck.name = "box_truck";
	truck.position.set(x, y, z);
	truck.rotation.y = yaw;
	parent.add(truck);
	addBox(truck, "truck_box", [8.5, 3.8, 3.4], [1.2, 2.1, 0], mats.truckYellow);
	addBox(truck, "truck_cab", [3.2, 2.8, 3.2], [-5.0, 1.75, 0], mats.carWhite);
	addBox(truck, "truck_windshield", [0.35, 1.1, 2.4], [-6.72, 2.2, 0], mats.glass);
	for (const wx of [-4.8, -1.2, 4.6]) {
		for (const wz of [-1.9, 1.9]) {
			addCylinder(truck, `truck_wheel_${wx}_${wz}`, 0.72, 0.5, [wx, 0.55, wz], mats.black).rotation.x = Math.PI / 2;
		}
	}
}

function buildSpeedboat(parent, mats, x, y, z, yaw, accentMat) {
	const boat = new THREE.Group();
	boat.name = "speedboat";
	boat.position.set(x, y, z);
	boat.rotation.y = yaw;
	parent.add(boat);
	addBox(boat, "boat_hull", [7.8, 1.1, 2.8], [0, 0, 0], mats.boatWhite);
	addBox(boat, "boat_deck", [5.8, 0.55, 2.2], [-0.4, 0.85, 0], accentMat);
	addBox(boat, "boat_windshield", [1.6, 0.7, 2.0], [-1.6, 1.45, 0], mats.glass);
	addBox(boat, "outboard_motor", [0.7, 1.0, 0.8], [4.3, 0.6, 0], mats.black);
}

function buildJetSki(parent, mats, x, y, z, yaw) {
	const ski = new THREE.Group();
	ski.name = "jet_ski";
	ski.position.set(x, y, z);
	ski.rotation.y = yaw;
	parent.add(ski);
	addBox(ski, "jetski_hull", [3.5, 0.7, 1.2], [0, 0, 0], mats.carCoral);
	addBox(ski, "jetski_seat", [1.8, 0.45, 0.65], [0.2, 0.65, 0], mats.black);
	addBox(ski, "jetski_handle", [0.25, 0.8, 0.9], [-1.1, 1.0, 0], mats.chrome);
}

function buildPatrolBoat(parent, mats, x, y, z, yaw) {
	const patrol = new THREE.Group();
	patrol.name = "patrol_boat";
	patrol.position.set(x, y, z);
	patrol.rotation.y = yaw;
	parent.add(patrol);
	addBox(patrol, "patrol_hull", [10.5, 1.4, 3.4], [0, 0, 0], mats.patrolNavy);
	addBox(patrol, "patrol_cabin", [3.8, 2.2, 2.6], [-1.6, 1.6, 0], mats.boatWhite);
	addBox(patrol, "patrol_glass", [0.35, 1.2, 2.1], [-3.6, 1.8, 0], mats.glass);
	addSphere(patrol, "patrol_blue_light", 0.35, [-1.6, 3.0, -0.55], mats.blueLight);
	addSphere(patrol, "patrol_red_light", 0.35, [-1.6, 3.0, 0.55], mats.red);
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
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 18, 12), mat);
	return place(parent, mesh, name, position);
}

function place(parent, mesh, name, position) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	parent.add(mesh);
	return mesh;
}
