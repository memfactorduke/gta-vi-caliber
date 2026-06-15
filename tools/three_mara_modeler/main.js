import * as THREE from "three";
import { GLTFLoader } from "three/addons/loaders/GLTFLoader.js";
import { OrbitControls } from "three/addons/controls/OrbitControls.js";

const canvas = document.querySelector("#mara-canvas");
const assetSelect = document.querySelector("#asset-select");
const assetStatus = document.querySelector("#asset-status");
const meshStats = document.querySelector("#mesh-stats");
const scaleControl = document.querySelector("#model-scale");
const keyLightControl = document.querySelector("#key-light");
const wireframeToggle = document.querySelector("#wireframe-toggle");
const rigToggle = document.querySelector("#rig-toggle");
const proxyToggle = document.querySelector("#proxy-toggle");
const turntableToggle = document.querySelector("#turntable-toggle");
const readout = document.querySelector("#readout");

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x090a0c);
scene.fog = new THREE.Fog(0x090a0c, 8, 19);

const renderer = new THREE.WebGLRenderer({
	canvas,
	antialias: true,
	powerPreference: "high-performance"
});
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.08;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;

const camera = new THREE.PerspectiveCamera(42, 1, 0.05, 100);
camera.position.set(0.15, 1.45, 4.2);

const controls = new OrbitControls(camera, renderer.domElement);
controls.target.set(0, 1.05, 0);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.minDistance = 2.2;
controls.maxDistance = 7.5;

const root = new THREE.Group();
scene.add(root);

const assetRoot = new THREE.Group();
assetRoot.name = "image_derived_asset";
root.add(assetRoot);

const proxyRoot = new THREE.Group();
proxyRoot.name = "game_ready_proxy";
root.add(proxyRoot);

const rigRoot = new THREE.Group();
rigRoot.name = "retopo_rig_guide";
root.add(rigRoot);

const hemi = new THREE.HemisphereLight(0xdde8ff, 0x19110c, 1.4);
scene.add(hemi);

const keyLight = new THREE.DirectionalLight(0xffdfb0, Number(keyLightControl.value));
keyLight.position.set(-2.6, 4.6, 3.4);
keyLight.castShadow = true;
keyLight.shadow.mapSize.set(2048, 2048);
scene.add(keyLight);

const rimLight = new THREE.DirectionalLight(0x9fbfff, 1.1);
rimLight.position.set(2.6, 2.5, -3.2);
scene.add(rimLight);

const floor = new THREE.Mesh(
	new THREE.CircleGeometry(2.3, 96),
	new THREE.MeshStandardMaterial({
		color: 0x1b1d1f,
		roughness: 0.82,
		metalness: 0.0
	})
);
floor.rotation.x = -Math.PI / 2;
floor.receiveShadow = true;
scene.add(floor);

const materials = {
	skin: new THREE.MeshPhysicalMaterial({
		color: 0xffc99d,
		roughness: 0.48,
		sheen: 0.18
	}),
	jacket: new THREE.MeshPhysicalMaterial({
		color: 0x20272a,
		roughness: 0.38,
		clearcoat: 0.32,
		clearcoatRoughness: 0.45
	}),
	shirt: new THREE.MeshStandardMaterial({
		color: 0xd6d0c4,
		roughness: 0.72
	}),
	fabric: new THREE.MeshStandardMaterial({
		color: 0x879286,
		roughness: 0.78
	}),
	black: new THREE.MeshPhysicalMaterial({
		color: 0x070707,
		roughness: 0.34,
		clearcoat: 0.55
	}),
	gold: new THREE.MeshStandardMaterial({
		color: 0xd6a84c,
		roughness: 0.32,
		metalness: 0.75
	}),
	eye: new THREE.MeshStandardMaterial({
		color: 0x22150d,
		roughness: 0.42
	}),
	guide: new THREE.LineBasicMaterial({
		color: 0x69d2ff,
		transparent: true,
		opacity: 0.72
	})
};

let currentAsset = null;
let assetBounds = new THREE.Box3();
let assetCenter = new THREE.Vector3();
let assetMaterials = [];
let lastTime = 0;

buildProxy();
buildRigGuide();
loadAsset(assetSelect.value);
resize();
requestAnimationFrame(animate);

assetSelect.addEventListener("change", () => loadAsset(assetSelect.value));
scaleControl.addEventListener("input", updateScale);
keyLightControl.addEventListener("input", () => {
	keyLight.intensity = Number(keyLightControl.value);
});
wireframeToggle.addEventListener("change", updateWireframe);
rigToggle.addEventListener("change", () => {
	rigRoot.visible = rigToggle.checked;
});
proxyToggle.addEventListener("change", () => {
	proxyRoot.visible = proxyToggle.checked;
});
window.addEventListener("resize", resize);

function loadAsset(url) {
	assetStatus.textContent = "Loading " + url.split("/").pop();
	meshStats.textContent = "Mesh stats unavailable";
	assetMaterials = [];
	if (currentAsset) {
		assetRoot.remove(currentAsset);
		currentAsset.traverse((node) => {
			if (node.isMesh) {
				node.geometry.dispose();
			}
		});
		currentAsset = null;
	}
	new GLTFLoader().load(
		url,
		(gltf) => {
			currentAsset = gltf.scene;
			currentAsset.name = "mara_source_mesh";
			assetRoot.add(currentAsset);
			prepareAsset(currentAsset);
			updateScale();
			updateWireframe();
			assetStatus.textContent = "Loaded " + url.split("/").pop();
		},
		(event) => {
			if (event.total > 0) {
				const pct = Math.round((event.loaded / event.total) * 100);
				assetStatus.textContent = "Loading " + pct + "%";
			}
		},
		(error) => {
			assetStatus.textContent = "Could not load GLB";
			readout.innerHTML =
				"<p>Serve the repo root over HTTP so Three.js can load /game/assets/characters/*.glb.</p>";
			console.error(error);
		}
	);
}

function prepareAsset(node) {
	let vertices = 0;
	let triangles = 0;
	node.traverse((child) => {
		if (!child.isMesh) {
			return;
		}
		child.castShadow = true;
		child.receiveShadow = true;
		child.frustumCulled = false;
		vertices += child.geometry.attributes.position?.count ?? 0;
		triangles += child.geometry.index ? child.geometry.index.count / 3 : vertices / 3;
		if (Array.isArray(child.material)) {
			for (const material of child.material) {
				assetMaterials.push(material);
			}
		} else if (child.material) {
			assetMaterials.push(child.material);
		}
	});
	assetBounds.setFromObject(node);
	assetBounds.getCenter(assetCenter);
	const size = new THREE.Vector3();
	assetBounds.getSize(size);
	node.position.sub(assetCenter);
	node.position.y += size.y * 0.5;
	const height = Math.max(size.y, 0.001);
	const fitScale = 1.78 / height;
	node.userData.fitScale = fitScale;
	meshStats.textContent = `${Math.round(vertices).toLocaleString()} verts / ${Math.round(triangles).toLocaleString()} tris`;
	readout.innerHTML =
		"<p>Loaded image-derived geometry. Use this view to judge silhouette, then retopo and skin to the blue rig guide before replacing the Godot player mesh.</p>";
}

function updateScale() {
	const value = Number(scaleControl.value);
	if (currentAsset) {
		const fitScale = currentAsset.userData.fitScale ?? 1;
		currentAsset.scale.setScalar(fitScale * value);
	}
	proxyRoot.scale.setScalar(value);
	rigRoot.scale.setScalar(value);
}

function updateWireframe() {
	for (const material of assetMaterials) {
		material.wireframe = wireframeToggle.checked;
		material.needsUpdate = true;
	}
}

function buildProxy() {
	const addCapsule = (name, radius, length, position, rotation, material) => {
		const mesh = new THREE.Mesh(new THREE.CapsuleGeometry(radius, length, 16, 24), material);
		mesh.name = name;
		mesh.position.copy(position);
		mesh.rotation.set(rotation.x, rotation.y, rotation.z);
		mesh.castShadow = true;
		mesh.receiveShadow = true;
		proxyRoot.add(mesh);
		return mesh;
	};
	addBox("torso_jacket", new THREE.Vector3(0.42, 0.48, 0.2), new THREE.Vector3(0, 1.14, 0), new THREE.Vector3(0.02, 0, 0), materials.jacket);
	addBox("shirt_panel", new THREE.Vector3(0.2, 0.36, 0.026), new THREE.Vector3(0, 1.11, 0.115), new THREE.Vector3(0.02, 0, 0), materials.shirt);
	addBox("jacket_lapel_l", new THREE.Vector3(0.09, 0.32, 0.03), new THREE.Vector3(-0.08, 1.18, 0.132), new THREE.Vector3(0.06, 0, -0.18), materials.black);
	addBox("jacket_lapel_r", new THREE.Vector3(0.09, 0.32, 0.03), new THREE.Vector3(0.08, 1.18, 0.132), new THREE.Vector3(0.06, 0, 0.18), materials.black);
	addBox("belt", new THREE.Vector3(0.44, 0.035, 0.055), new THREE.Vector3(0, 0.86, 0.105), new THREE.Vector3(0, 0, 0), materials.black);
	addBox("pelvis_trousers", new THREE.Vector3(0.36, 0.24, 0.18), new THREE.Vector3(0, 0.78, 0), new THREE.Vector3(0, 0, 0), materials.fabric);
	addCapsule("head", 0.14, 0.05, new THREE.Vector3(0, 1.63, 0.01), new THREE.Vector3(0, 0, 0), materials.skin);
	addSphere("eye_l", 0.018, new THREE.Vector3(-0.052, 1.66, 0.145), materials.eye);
	addSphere("eye_r", 0.018, new THREE.Vector3(0.052, 1.66, 0.145), materials.eye);
	addSphere("nose", 0.024, new THREE.Vector3(0, 1.62, 0.16), materials.skin);
	addCapsule("upper_arm_l", 0.048, 0.38, new THREE.Vector3(-0.3, 1.16, 0), new THREE.Vector3(0.18, 0, 0.12), materials.jacket);
	addCapsule("upper_arm_r", 0.048, 0.38, new THREE.Vector3(0.3, 1.16, 0), new THREE.Vector3(0.18, 0, -0.12), materials.jacket);
	addCapsule("forearm_l", 0.044, 0.34, new THREE.Vector3(-0.36, 0.84, 0.02), new THREE.Vector3(0.12, 0, 0.02), materials.jacket);
	addCapsule("forearm_r", 0.044, 0.34, new THREE.Vector3(0.36, 0.84, 0.02), new THREE.Vector3(0.12, 0, -0.02), materials.jacket);
	addCapsule("thigh_l", 0.065, 0.42, new THREE.Vector3(-0.105, 0.55, 0), new THREE.Vector3(0.02, 0, 0), materials.fabric);
	addCapsule("thigh_r", 0.065, 0.42, new THREE.Vector3(0.105, 0.55, 0), new THREE.Vector3(0.02, 0, 0), materials.fabric);
	addCapsule("shin_l", 0.058, 0.42, new THREE.Vector3(-0.105, 0.23, 0.015), new THREE.Vector3(-0.03, 0, 0), materials.fabric);
	addCapsule("shin_r", 0.058, 0.42, new THREE.Vector3(0.105, 0.23, 0.015), new THREE.Vector3(-0.03, 0, 0), materials.fabric);
	addCapsule("boot_l", 0.067, 0.22, new THREE.Vector3(-0.105, 0.06, 0.085), new THREE.Vector3(Math.PI / 2, 0, 0), materials.black);
	addCapsule("boot_r", 0.067, 0.22, new THREE.Vector3(0.105, 0.06, 0.085), new THREE.Vector3(Math.PI / 2, 0, 0), materials.black);

	addBox("cross_body_strap", new THREE.Vector3(0.035, 0.78, 0.026), new THREE.Vector3(0.02, 1.18, 0.145), new THREE.Vector3(0.02, 0, -0.52), materials.black);

	const pendant = new THREE.Mesh(new THREE.SphereGeometry(0.04, 24, 16), materials.gold);
	pendant.name = "pendant";
	pendant.position.set(0, 1.02, 0.25);
	pendant.castShadow = true;
	proxyRoot.add(pendant);

	const mouth = new THREE.Mesh(new THREE.CylinderGeometry(0.006, 0.006, 0.07, 12), materials.black);
	mouth.name = "mouth";
	mouth.position.set(0, 1.58, 0.15);
	mouth.rotation.set(Math.PI / 2, 0, Math.PI / 2);
	proxyRoot.add(mouth);
}

function addBox(name, size, position, rotation, material) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size.x, size.y, size.z), material);
	mesh.name = name;
	mesh.position.copy(position);
	mesh.rotation.set(rotation.x, rotation.y, rotation.z);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	proxyRoot.add(mesh);
	return mesh;
}

function addSphere(name, radius, position, material) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 24, 16), material);
	mesh.name = name;
	mesh.position.copy(position);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	proxyRoot.add(mesh);
	return mesh;
}

function buildRigGuide() {
	const points = [
		[0, 1.72, 0],
		[0, 1.48, 0],
		[0, 1.1, 0],
		[0, 0.78, 0],
		[-0.34, 1.42, 0],
		[-0.48, 1.08, 0],
		[-0.43, 0.72, 0],
		[0.34, 1.42, 0],
		[0.48, 1.08, 0],
		[0.43, 0.72, 0],
		[-0.14, 0.78, 0],
		[-0.16, 0.42, 0],
		[-0.14, 0.08, 0],
		[0.14, 0.78, 0],
		[0.16, 0.42, 0],
		[0.14, 0.08, 0]
	].map(([x, y, z]) => new THREE.Vector3(x, y, z));
	const bones = [
		[0, 1],
		[1, 2],
		[2, 3],
		[1, 4],
		[4, 5],
		[5, 6],
		[1, 7],
		[7, 8],
		[8, 9],
		[3, 10],
		[10, 11],
		[11, 12],
		[3, 13],
		[13, 14],
		[14, 15]
	];
	for (const [a, b] of bones) {
		const geometry = new THREE.BufferGeometry().setFromPoints([points[a], points[b]]);
		rigRoot.add(new THREE.Line(geometry, materials.guide));
	}
	for (const point of points) {
		const joint = new THREE.Mesh(
			new THREE.SphereGeometry(0.018, 12, 8),
			new THREE.MeshBasicMaterial({ color: 0x69d2ff })
		);
		joint.position.copy(point);
		rigRoot.add(joint);
	}
}

function animate(time) {
	const seconds = time * 0.001;
	const delta = Math.min(seconds - lastTime, 0.05);
	lastTime = seconds;
	if (turntableToggle.checked) {
		root.rotation.y += delta * 0.28;
	}
	const breath = Math.sin(seconds * Math.PI * 0.66) * 0.012;
	proxyRoot.position.y = breath;
	rigRoot.position.y = breath;
	controls.update();
	renderer.render(scene, camera);
	requestAnimationFrame(animate);
}

function resize() {
	const width = canvas.clientWidth || window.innerWidth;
	const height = canvas.clientHeight || window.innerHeight;
	renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
	renderer.setSize(width, height, false);
	camera.aspect = width / Math.max(height, 1);
	camera.updateProjectionMatrix();
}
