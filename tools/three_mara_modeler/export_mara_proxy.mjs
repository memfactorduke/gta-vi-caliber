import { writeFile } from "node:fs/promises";
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
const outputPath = resolve(repoRoot, "game/assets/characters/mara_three_proxy.glb");

const scene = new THREE.Scene();
scene.name = "mara_three_proxy";

const materials = {
	skin: material("mara_skin", 0xffc99d, 0.48, 0.0, 0.0),
	jacket: material("mara_clearcoat_jacket", 0x20272a, 0.42, 0.0, 0.35),
	shirt: material("mara_layered_shirt", 0xd6d0c4, 0.72, 0.0, 0.0),
	fabric: material("mara_sage_fabric", 0x879286, 0.78, 0.0, 0.0),
	black: material("mara_black_leather", 0x080808, 0.36, 0.0, 0.5),
	gold: material("mara_brass_pendant", 0xd6a84c, 0.3, 0.75, 0.0),
	hair: material("mara_dark_hair", 0x161312, 0.5, 0.0, 0.18),
	eye: material("mara_deep_brown_eyes", 0x22150d, 0.42, 0.0, 0.0)
};

buildMara(scene, materials);

const exporter = new GLTFExporter();
const glb = await exporter.parseAsync(scene, {
	binary: true,
	onlyVisible: true,
	trs: false
});

await writeFile(outputPath, Buffer.from(glb));
console.log(`exported ${outputPath}`);

function material(name, color, roughness, metalness, clearcoat) {
	const mat = new THREE.MeshPhysicalMaterial({
		name,
		color,
		roughness,
		metalness,
		clearcoat,
		clearcoatRoughness: 0.46
	});
	mat.userData.godot_hint = "Three.js-authored original Mara proxy material";
	return mat;
}

function buildMara(root, mats) {
	const proxy = new THREE.Group();
	proxy.name = "MaraThreeProxy";
	proxy.rotation.y = Math.PI;
	root.add(proxy);

	addBox(proxy, "torso_jacket", [0.42, 0.48, 0.2], [0, 1.14, 0], [0.02, 0, 0], mats.jacket);
	addBox(proxy, "shirt_panel", [0.2, 0.36, 0.026], [0, 1.11, 0.115], [0.02, 0, 0], mats.shirt);
	addBox(proxy, "jacket_lapel_l", [0.09, 0.32, 0.03], [-0.08, 1.18, 0.132], [0.06, 0, -0.18], mats.black);
	addBox(proxy, "jacket_lapel_r", [0.09, 0.32, 0.03], [0.08, 1.18, 0.132], [0.06, 0, 0.18], mats.black);
	addBox(proxy, "belt", [0.44, 0.035, 0.055], [0, 0.86, 0.105], [0, 0, 0], mats.black);
	addBox(proxy, "pelvis_trousers", [0.36, 0.24, 0.18], [0, 0.78, 0], [0, 0, 0], mats.fabric);
	addCapsule(proxy, "neck", 0.06, 0.06, [0, 1.5, 0.01], [0, 0, 0], mats.skin);
	addCapsule(proxy, "head", 0.145, 0.06, [0, 1.64, 0.015], [0, 0, 0], mats.skin);
	addCapsule(proxy, "hair_mass", 0.135, 0.1, [0, 1.69, -0.045], [-0.18, 0, 0], mats.hair);
	addCapsule(proxy, "cap", 0.15, 0.06, [0, 1.76, 0.025], [Math.PI / 2, 0, 0], mats.black);
	addSphere(proxy, "eye_l", 0.018, [-0.052, 1.66, 0.145], mats.eye);
	addSphere(proxy, "eye_r", 0.018, [0.052, 1.66, 0.145], mats.eye);
	addSphere(proxy, "nose", 0.024, [0, 1.62, 0.16], mats.skin);
	addCylinder(proxy, "mouth", 0.006, 0.07, [0, 1.58, 0.15], [Math.PI / 2, 0, Math.PI / 2], mats.black);

	addCapsule(proxy, "upper_arm_l", 0.048, 0.38, [-0.3, 1.16, 0], [0.18, 0, 0.12], mats.jacket);
	addCapsule(proxy, "upper_arm_r", 0.048, 0.38, [0.3, 1.16, 0], [0.18, 0, -0.12], mats.jacket);
	addCapsule(proxy, "forearm_l", 0.044, 0.34, [-0.36, 0.84, 0.02], [0.12, 0, 0.02], mats.jacket);
	addCapsule(proxy, "forearm_r", 0.044, 0.34, [0.36, 0.84, 0.02], [0.12, 0, -0.02], mats.jacket);
	addCapsule(proxy, "hand_l", 0.052, 0.06, [-0.43, 0.58, 0.04], [0, 0, 0], mats.skin);
	addCapsule(proxy, "hand_r", 0.052, 0.06, [0.43, 0.58, 0.04], [0, 0, 0], mats.skin);

	addCapsule(proxy, "thigh_l", 0.065, 0.42, [-0.105, 0.55, 0], [0.02, 0, 0], mats.fabric);
	addCapsule(proxy, "thigh_r", 0.065, 0.42, [0.105, 0.55, 0], [0.02, 0, 0], mats.fabric);
	addCapsule(proxy, "shin_l", 0.058, 0.42, [-0.105, 0.23, 0.015], [-0.03, 0, 0], mats.fabric);
	addCapsule(proxy, "shin_r", 0.058, 0.42, [0.105, 0.23, 0.015], [-0.03, 0, 0], mats.fabric);
	addCapsule(proxy, "boot_l", 0.067, 0.22, [-0.105, 0.06, 0.085], [Math.PI / 2, 0, 0], mats.black);
	addCapsule(proxy, "boot_r", 0.067, 0.22, [0.105, 0.06, 0.085], [Math.PI / 2, 0, 0], mats.black);

	addBox(proxy, "cross_body_strap", [0.035, 0.78, 0.026], [0.02, 1.18, 0.145], [0.02, 0, -0.52], mats.black);
	addCylinder(proxy, "pendant_cord", 0.007, 0.32, [0, 1.18, 0.235], [0, 0, 0], mats.black);
	addSphere(proxy, "pendant", 0.045, [0, 1.0, 0.245], mats.gold);
	addCylinder(proxy, "thigh_band_l", 0.014, 0.2, [-0.105, 0.51, 0.067], [Math.PI / 2, 0, Math.PI / 2], mats.black);
	addCylinder(proxy, "thigh_band_r", 0.014, 0.2, [0.105, 0.51, 0.067], [Math.PI / 2, 0, Math.PI / 2], mats.black);
	addSphere(proxy, "earring_l", 0.018, [-0.135, 1.61, 0.03], mats.gold);

	proxy.traverse((node) => {
		if (!node.isMesh) {
			return;
		}
		node.castShadow = true;
		node.receiveShadow = true;
		node.userData.mara_three_proxy = true;
	});
}

function addCapsule(parent, name, radius, length, position, rotation, mat) {
	const mesh = new THREE.Mesh(new THREE.CapsuleGeometry(radius, length, 16, 24), mat);
	place(parent, mesh, name, position, rotation);
	return mesh;
}

function addCylinder(parent, name, radius, length, position, rotation, mat) {
	const mesh = new THREE.Mesh(new THREE.CylinderGeometry(radius, radius, length, 18), mat);
	place(parent, mesh, name, position, rotation);
	return mesh;
}

function addBox(parent, name, size, position, rotation, mat) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), mat);
	place(parent, mesh, name, position, rotation);
	return mesh;
}

function addSphere(parent, name, radius, position, mat) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 24, 16), mat);
	place(parent, mesh, name, position, [0, 0, 0]);
	return mesh;
}

function place(parent, mesh, name, position, rotation) {
	mesh.name = name;
	mesh.position.set(position[0], position[1], position[2]);
	mesh.rotation.set(rotation[0], rotation[1], rotation[2]);
	parent.add(mesh);
}
