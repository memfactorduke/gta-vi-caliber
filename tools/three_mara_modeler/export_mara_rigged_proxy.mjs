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
const outputPath = resolve(repoRoot, "game/assets/characters/mara_three_rigged_proxy.glb");

const scene = new THREE.Scene();
scene.name = "mara_three_rigged_proxy";

const materials = {
	skin: material("mara_rigged_skin", 0xffc49a, 0.46, 0.0, 0.08),
	jacket: material("mara_rigged_clearcoat_jacket", 0x1f2729, 0.44, 0.0, 0.3),
	jacketTrim: material("mara_rigged_jacket_trim", 0x0b0d0e, 0.38, 0.0, 0.36),
	shirt: material("mara_rigged_layered_shirt", 0xd8d0c2, 0.74, 0.0, 0.0),
	fabric: material("mara_rigged_sage_trousers", 0x78887c, 0.8, 0.0, 0.0),
	leather: material("mara_rigged_black_leather", 0x070707, 0.34, 0.0, 0.5),
	metal: material("mara_rigged_worn_brass", 0x8f7a50, 0.42, 0.78, 0.08),
	hair: material("mara_rigged_dark_hair", 0x171312, 0.52, 0.0, 0.16),
	eye: material("mara_rigged_brown_eyes", 0x24160d, 0.4, 0.0, 0.0),
	sclera: material("mara_rigged_eye_sclera", 0xf2e6d8, 0.34, 0.0, 0.18),
	softShadow: material("mara_rigged_soft_skin_shadow", 0x8c5d50, 0.62, 0.0, 0.0),
	eyeWet: material("mara_rigged_eye_wetline", 0xf6eee5, 0.2, 0.0, 0.7)
};

buildRiggedMara(scene, materials);

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
		clearcoatRoughness: 0.4
	});
	mat.userData.godot_hint = "Three.js-authored original Mara rigged proxy material";
	return mat;
}

function buildRiggedMara(root, mats) {
	const proxy = new THREE.Group();
	proxy.name = "MaraRiggedProxy";
	root.add(proxy);

	const rig = createSkeleton();
	proxy.add(rig.bones.MaraRoot);

	const skeleton = new THREE.Skeleton(rig.orderedBones);
	const skinnedMeshes = [
		skinnedTube("mara_rigged_jacket", [
			ring([0, 0.82, 0.006], 0.215, 0.1, "MaraHips"),
			ring([0, 0.94, -0.002], 0.232, 0.112, "MaraHips"),
			ring([0, 1.08, -0.012], 0.246, 0.126, "MaraSpine"),
			ring([0, 1.24, -0.018], 0.238, 0.138, "MaraChest"),
			ring([0, 1.39, -0.014], 0.205, 0.123, "MaraChest"),
			ring([0, 1.47, -0.006], 0.165, 0.098, "MaraChest")
		], rig.index, mats.jacket, 32),
		skinnedGarmentPanel("mara_rigged_jacket_front_panel_l", [
			garmentRow([-0.105, 0.86, -0.13], 0.085, 0.012, "MaraHips"),
			garmentRow([-0.1, 1.02, -0.151], 0.092, 0.016, "MaraSpine"),
			garmentRow([-0.085, 1.22, -0.164], 0.078, 0.018, "MaraChest"),
			garmentRow([-0.058, 1.42, -0.144], 0.054, 0.012, "MaraChest")
		], rig.index, mats.jacket, 7),
		skinnedGarmentPanel("mara_rigged_jacket_front_panel_r", [
			garmentRow([0.105, 0.86, -0.13], 0.085, 0.012, "MaraHips"),
			garmentRow([0.1, 1.02, -0.151], 0.092, 0.016, "MaraSpine"),
			garmentRow([0.085, 1.22, -0.164], 0.078, 0.018, "MaraChest"),
			garmentRow([0.058, 1.42, -0.144], 0.054, 0.012, "MaraChest")
		], rig.index, mats.jacket, 7),
		skinnedTube("mara_rigged_shirt_panel", [
			ring([0, 0.92, -0.11], 0.105, 0.02, "MaraHips"),
			ring([0, 1.15, -0.135], 0.12, 0.025, "MaraSpine"),
			ring([0, 1.38, -0.13], 0.1, 0.022, "MaraChest")
		], rig.index, mats.shirt, 8),
		skinnedGarmentPanel("mara_rigged_shirt_draped_front", [
			garmentRow([0, 0.9, -0.146], 0.095, 0.01, "MaraHips"),
			garmentRow([0, 1.08, -0.165], 0.108, 0.014, "MaraSpine"),
			garmentRow([0, 1.27, -0.169], 0.095, 0.012, "MaraChest"),
			garmentRow([0, 1.42, -0.148], 0.07, 0.008, "MaraChest")
		], rig.index, mats.shirt, 7),
		skinnedTube("mara_rigged_trousers", [
			ring([0, 0.58, 0.002], 0.17, 0.078, "MaraHips"),
			ring([0, 0.7, 0], 0.205, 0.096, "MaraHips"),
			ring([0, 0.83, -0.004], 0.218, 0.11, "MaraHips")
		], rig.index, mats.fabric, 28),
		skinnedGarmentPanel("mara_rigged_trouser_front_panel_l", [
			garmentRow([-0.094, 0.83, -0.116], 0.068, 0.01, "MaraHips"),
			garmentRow([-0.102, 0.68, -0.112], 0.076, 0.012, "MaraHipL"),
			garmentRow([-0.112, 0.5, -0.098], 0.07, 0.01, "MaraHipL"),
			garmentRow([-0.115, 0.34, -0.079], 0.054, 0.006, "MaraKneeL")
		], rig.index, mats.fabric, 6),
		skinnedGarmentPanel("mara_rigged_trouser_front_panel_r", [
			garmentRow([0.094, 0.83, -0.116], 0.068, 0.01, "MaraHips"),
			garmentRow([0.102, 0.68, -0.112], 0.076, 0.012, "MaraHipR"),
			garmentRow([0.112, 0.5, -0.098], 0.07, 0.01, "MaraHipR"),
			garmentRow([0.115, 0.34, -0.079], 0.054, 0.006, "MaraKneeR")
		], rig.index, mats.fabric, 6),
		skinnedTube("mara_rigged_neck", [
			ring([0, 1.43, 0.012], 0.048, 0.039, "MaraNeck"),
			ring([0, 1.53, 0.014], 0.052, 0.043, "MaraNeck")
		], rig.index, mats.skin),
		skinnedEllipsoid("mara_rigged_head", [0, 1.64, -0.012], [0.116, 0.136, 0.088], "MaraHead", rig.index, mats.skin, 24, 18),
		skinnedFaceMask("mara_rigged_skin_face_mask", rig.index, mats.skin),
		skinnedTube("mara_rigged_hair_mass", [
			ring([0, 1.58, 0.125], 0.11, 0.026, "MaraHead"),
			ring([0, 1.67, 0.13], 0.14, 0.034, "MaraHead"),
			ring([0, 1.75, 0.105], 0.118, 0.03, "MaraHead"),
			ring([0, 1.79, 0.08], 0.078, 0.02, "MaraHead")
		], rig.index, mats.hair, 28),
		skinnedLimb("mara_rigged_upper_arm_l", [-0.2, 1.22, 0], [-0.255, 0.98, 0.015], [[0.052, 0.037], [0.048, 0.034], [0.038, 0.03]], "MaraShoulderL", "MaraElbowL", rig.index, mats.jacket),
		skinnedLimb("mara_rigged_upper_arm_r", [0.2, 1.22, 0], [0.255, 0.98, 0.015], [[0.052, 0.037], [0.048, 0.034], [0.038, 0.03]], "MaraShoulderR", "MaraElbowR", rig.index, mats.jacket),
		skinnedLimb("mara_rigged_forearm_l", [-0.255, 0.98, 0.015], [-0.305, 0.76, 0.03], [[0.04, 0.032], [0.043, 0.03], [0.032, 0.024]], "MaraElbowL", "MaraHandL", rig.index, mats.jacket),
		skinnedLimb("mara_rigged_forearm_r", [0.255, 0.98, 0.015], [0.305, 0.76, 0.03], [[0.04, 0.032], [0.043, 0.03], [0.032, 0.024]], "MaraElbowR", "MaraHandR", rig.index, mats.jacket),
		skinnedLimb("mara_rigged_hand_l", [-0.305, 0.76, 0.03], [-0.325, 0.68, 0.045], [[0.032, 0.022], [0.038, 0.021], [0.026, 0.016]], "MaraHandL", "MaraHandL", rig.index, mats.skin, 8),
		skinnedLimb("mara_rigged_hand_r", [0.305, 0.76, 0.03], [0.325, 0.68, 0.045], [[0.032, 0.022], [0.038, 0.021], [0.026, 0.016]], "MaraHandR", "MaraHandR", rig.index, mats.skin, 8),
		skinnedLimb("mara_rigged_thigh_l", [-0.105, 0.72, 0], [-0.115, 0.38, 0.018], [[0.076, 0.055], [0.09, 0.061], [0.064, 0.048]], "MaraHipL", "MaraKneeL", rig.index, mats.fabric),
		skinnedLimb("mara_rigged_thigh_r", [0.105, 0.72, 0], [0.115, 0.38, 0.018], [[0.076, 0.055], [0.09, 0.061], [0.064, 0.048]], "MaraHipR", "MaraKneeR", rig.index, mats.fabric),
		skinnedLimb("mara_rigged_shin_l", [-0.115, 0.38, 0.018], [-0.11, 0.09, 0.045], [[0.066, 0.049], [0.059, 0.044], [0.047, 0.037]], "MaraKneeL", "MaraAnkleL", rig.index, mats.fabric),
		skinnedLimb("mara_rigged_shin_r", [0.115, 0.38, 0.018], [0.11, 0.09, 0.045], [[0.066, 0.049], [0.059, 0.044], [0.047, 0.037]], "MaraKneeR", "MaraAnkleR", rig.index, mats.fabric),
		skinnedLimb("mara_rigged_boot_l", [-0.11, 0.09, -0.045], [-0.11, 0.045, -0.185], [[0.063, 0.045], [0.072, 0.041], [0.083, 0.052]], "MaraAnkleL", "MaraToeL", rig.index, mats.leather, 10),
		skinnedLimb("mara_rigged_boot_r", [0.11, 0.09, -0.045], [0.11, 0.045, -0.185], [[0.063, 0.045], [0.072, 0.041], [0.083, 0.052]], "MaraAnkleR", "MaraToeR", rig.index, mats.leather, 10)
	];

	for (const mesh of skinnedMeshes) {
		mesh.bind(skeleton);
		mesh.castShadow = true;
		mesh.receiveShadow = true;
		mesh.userData.mara_three_rigged_proxy = true;
		proxy.add(mesh);
	}

	addEllipsoidDetail(proxy, "mara_rigged_eye_socket_shadow_l", [0.046, 1.656, -0.151], [0.029, 0.012, 0.004], mats.softShadow, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_socket_shadow_r", [-0.046, 1.656, -0.151], [0.029, 0.012, 0.004], mats.softShadow, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_sclera_l", [0.046, 1.657, -0.159], [0.015, 0.0062, 0.003], mats.sclera, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_sclera_r", [-0.046, 1.657, -0.159], [0.015, 0.0062, 0.003], mats.sclera, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_l", [0.046, 1.657, -0.164], [0.0045, 0.0048, 0.0016], mats.eye, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_r", [-0.046, 1.657, -0.164], [0.0045, 0.0048, 0.0016], mats.eye, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_iris_l", [0.046, 1.657, -0.163], [0.0065, 0.006, 0.0018], mats.eye, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_iris_r", [-0.046, 1.657, -0.163], [0.0065, 0.006, 0.0018], mats.eye, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_highlight_l", [0.049, 1.661, -0.166], [0.0025, 0.0018, 0.0009], mats.eyeWet, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_eye_highlight_r", [-0.043, 1.661, -0.166], [0.0025, 0.0018, 0.0009], mats.eyeWet, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_cornea_glint_l", [0.043, 1.659, -0.167], [0.0017, 0.0013, 0.0007], mats.eyeWet, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_cornea_glint_r", [-0.049, 1.659, -0.167], [0.0017, 0.0013, 0.0007], mats.eyeWet, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_upper_eyelid_l", [0.046, 1.664, -0.148], [0.024, 0.007, 0.005], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_upper_eyelid_r", [-0.046, 1.664, -0.148], [0.024, 0.007, 0.005], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_lower_eyelid_l", [0.046, 1.65, -0.148], [0.023, 0.0045, 0.0045], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_lower_eyelid_r", [-0.046, 1.65, -0.148], [0.023, 0.0045, 0.0045], mats.skin, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_lid_crease_l", [[0.024, 1.673, -0.151], [0.046, 1.678, -0.154], [0.07, 1.672, -0.15]], 0.0015, mats.softShadow, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_lid_crease_r", [[-0.024, 1.673, -0.151], [-0.046, 1.678, -0.154], [-0.07, 1.672, -0.15]], 0.0015, mats.softShadow, rig.bones.MaraHead);
	addAttachedBox(proxy, "mara_rigged_lash_l", [0.046, 1.671, -0.153], [0.033, 0.0016, 0.0022], mats.hair, rig.bones.MaraHead, [0.0, 0.0, 0.06]);
	addAttachedBox(proxy, "mara_rigged_lash_r", [-0.046, 1.671, -0.153], [0.033, 0.0016, 0.0022], mats.hair, rig.bones.MaraHead, [0.0, 0.0, -0.06]);
	addEllipsoidDetail(proxy, "mara_rigged_skin_brow_ridge_l", [0.046, 1.686, -0.14], [0.04, 0.007, 0.008], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_brow_ridge_r", [-0.046, 1.686, -0.14], [0.04, 0.007, 0.008], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_under_eye_shadow_l", [0.046, 1.635, -0.143], [0.032, 0.006, 0.004], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_under_eye_shadow_r", [-0.046, 1.635, -0.143], [0.032, 0.006, 0.004], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_nose_bridge", [0, 1.642, -0.154], [0.018, 0.045, 0.015], mats.skin, rig.bones.MaraHead);
	addRigidDetail(proxy, "mara_rigged_nose_tip", [0, 1.61, -0.166], 0.021, mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_nostril_l", [0.014, 1.602, -0.184], [0.008, 0.004, 0.003], mats.jacketTrim, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_nostril_r", [-0.014, 1.602, -0.184], [0.008, 0.004, 0.003], mats.jacketTrim, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_cheek_l", [0.07, 1.615, -0.146], [0.029, 0.018, 0.008], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_cheek_r", [-0.07, 1.615, -0.146], [0.029, 0.018, 0.008], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_nasolabial_fold_l", [0.039, 1.593, -0.158], [0.006, 0.026, 0.004], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_nasolabial_fold_r", [-0.039, 1.593, -0.158], [0.006, 0.026, 0.004], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_temple_l", [0.098, 1.657, -0.092], [0.018, 0.032, 0.006], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_temple_r", [-0.098, 1.657, -0.092], [0.018, 0.032, 0.006], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_jaw_shadow_l", [0.063, 1.552, -0.13], [0.036, 0.01, 0.006], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_jaw_shadow_r", [-0.063, 1.552, -0.13], [0.036, 0.01, 0.006], mats.skin, rig.bones.MaraHead);
	addRigidDetail(proxy, "mara_rigged_pendant", [0, 1.01, -0.245], 0.032, mats.metal, rig.bones.MaraSpine);
	addRigidDetail(proxy, "mara_rigged_ear_l", [0.136, 1.63, -0.01], 0.026, mats.skin, rig.bones.MaraHead);
	addRigidDetail(proxy, "mara_rigged_ear_r", [-0.136, 1.63, -0.01], 0.026, mats.skin, rig.bones.MaraHead);
	addRigidDetail(proxy, "mara_rigged_earring_l", [0.141, 1.588, -0.018], 0.01, mats.metal, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_mouth_upper_lip", [0, 1.591, -0.164], [0.036, 0.0045, 0.004], mats.jacketTrim, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_mouth_lower_lip", [0, 1.579, -0.163], [0.033, 0.006, 0.004], mats.jacketTrim, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_mouth", [[-0.029, 1.582, -0.168], [-0.011, 1.58, -0.17], [0, 1.58, -0.171], [0.011, 1.58, -0.17], [0.029, 1.582, -0.168]], 0.0019, mats.jacketTrim, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_mouth_soft_seam", [[-0.025, 1.586, -0.166], [-0.009, 1.588, -0.168], [0.009, 1.588, -0.168], [0.025, 1.586, -0.166]], 0.0012, mats.softShadow, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_mouth_corner_l", [0.034, 1.583, -0.166], [0.006, 0.004, 0.003], mats.jacketTrim, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_mouth_corner_r", [-0.034, 1.583, -0.166], [0.006, 0.004, 0.003], mats.jacketTrim, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_philtrum_l", [[0.007, 1.603, -0.161], [0.005, 1.596, -0.165], [0.003, 1.591, -0.167]], 0.0012, mats.softShadow, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_philtrum_r", [[-0.007, 1.603, -0.161], [-0.005, 1.596, -0.165], [-0.003, 1.591, -0.167]], 0.0012, mats.softShadow, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_skin_under_lip_plane", [0, 1.568, -0.154], [0.032, 0.006, 0.004], mats.skin, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_chin", [0, 1.545, -0.148], [0.04, 0.014, 0.008], mats.skin, rig.bones.MaraHead);
	addAttachedBox(proxy, "mara_rigged_brow_l", [0.052, 1.695, -0.153], [0.056, 0.008, 0.008], mats.hair, rig.bones.MaraHead, [0, 0, 0.1]);
	addAttachedBox(proxy, "mara_rigged_brow_r", [-0.052, 1.695, -0.153], [0.056, 0.008, 0.008], mats.hair, rig.bones.MaraHead, [0, 0, -0.1]);
	addEllipsoidDetail(proxy, "mara_rigged_hair_crown", [0, 1.758, -0.012], [0.118, 0.03, 0.074], mats.hair, rig.bones.MaraHead);
	addAttachedBox(proxy, "mara_rigged_hairline", [0, 1.716, -0.12], [0.15, 0.012, 0.01], mats.hair, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_hair_forelock", [0.032, 1.721, -0.119], [0.038, 0.007, 0.007], mats.hair, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_hair_sideburn_l", [0.119, 1.625, -0.071], [0.014, 0.062, 0.012], mats.hair, rig.bones.MaraHead);
	addEllipsoidDetail(proxy, "mara_rigged_hair_sideburn_r", [-0.119, 1.625, -0.071], [0.014, 0.062, 0.012], mats.hair, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_hair_parting_line", [[-0.018, 1.739, -0.101], [0.018, 1.748, -0.101], [0.054, 1.734, -0.095]], 0.0011, mats.softShadow, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_hairline_wisps_l", [[0.026, 1.716, -0.125], [0.052, 1.711, -0.129], [0.071, 1.704, -0.121]], 0.0014, mats.hair, rig.bones.MaraHead);
	addAttachedTubeCurve(proxy, "mara_rigged_hairline_wisps_r", [[-0.024, 1.716, -0.125], [-0.049, 1.712, -0.129], [-0.068, 1.704, -0.121]], 0.0013, mats.hair, rig.bones.MaraHead);
	addAttachedCapsule(proxy, "mara_rigged_hair_lock_l", [0.116, 1.58, -0.045], 0.016, 0.13, mats.hair, rig.bones.MaraHead, [0.0, 0.0, -0.2]);
	addAttachedCapsule(proxy, "mara_rigged_hair_lock_r", [-0.116, 1.61, -0.04], 0.014, 0.1, mats.hair, rig.bones.MaraHead, [0.0, 0.0, 0.18]);
	addAttachedCapsule(proxy, "mara_rigged_hair_strand_l", [0.145, 1.57, -0.035], 0.012, 0.22, mats.hair, rig.bones.MaraHead, [0.04, 0.0, -0.3]);
	addAttachedCapsule(proxy, "mara_rigged_hair_strand_r", [-0.132, 1.59, -0.034], 0.01, 0.17, mats.hair, rig.bones.MaraHead, [0.02, 0.0, 0.26]);
	addAttachedCapsule(proxy, "mara_rigged_hair_flyaway_l", [0.092, 1.705, -0.125], 0.004, 0.145, mats.hair, rig.bones.MaraHead, [0.0, 0.18, -1.0]);
	addAttachedCapsule(proxy, "mara_rigged_hair_flyaway_r", [-0.088, 1.697, -0.126], 0.004, 0.125, mats.hair, rig.bones.MaraHead, [0.0, -0.16, 1.05]);
	addAttachedCapsule(proxy, "mara_rigged_hair_nape_layer_l", [0.075, 1.51, 0.08], 0.011, 0.16, mats.hair, rig.bones.MaraHead, [0.1, 0.0, -0.18]);
	addAttachedCapsule(proxy, "mara_rigged_hair_nape_layer_r", [-0.075, 1.51, 0.08], 0.011, 0.16, mats.hair, rig.bones.MaraHead, [0.1, 0.0, 0.18]);
	addAttachedBox(proxy, "mara_rigged_jacket_lapel_l", [0.068, 1.24, -0.19], [0.052, 0.32, 0.014], mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, -0.34]);
	addAttachedBox(proxy, "mara_rigged_jacket_lapel_r", [-0.068, 1.24, -0.19], [0.052, 0.32, 0.014], mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, 0.34]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_collar_l", [0.1, 1.43, -0.145], 0.012, 0.18, mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, 0.55]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_collar_r", [-0.1, 1.43, -0.145], 0.012, 0.18, mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, -0.55]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_zipper", [0, 1.17, -0.205], 0.005, 0.44, mats.metal, rig.bones.MaraChest);
	addAttachedBox(proxy, "mara_rigged_jacket_zipper_pull", [0, 1.31, -0.214], [0.011, 0.024, 0.004], mats.metal, rig.bones.MaraChest);
	addAttachedCapsule(proxy, "mara_rigged_jacket_seam_l", [0.19, 1.16, -0.105], 0.0045, 0.5, mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, 0.04]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_seam_r", [-0.19, 1.16, -0.105], 0.0045, 0.5, mats.jacketTrim, rig.bones.MaraChest, [0.0, 0.0, -0.04]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_topstitch_l", [0.114, 1.25, -0.205], 0.003, 0.34, mats.shirt, rig.bones.MaraChest, [0.0, 0.0, -0.32]);
	addAttachedCapsule(proxy, "mara_rigged_jacket_topstitch_r", [-0.114, 1.25, -0.205], 0.003, 0.34, mats.shirt, rig.bones.MaraChest, [0.0, 0.0, 0.32]);
	addAttachedCapsule(proxy, "mara_rigged_sleeve_wrinkle_l", [-0.27, 0.89, -0.012], 0.004, 0.13, mats.jacketTrim, rig.bones.MaraElbowL, [Math.PI / 2, 0.0, -0.18]);
	addAttachedCapsule(proxy, "mara_rigged_sleeve_wrinkle_r", [0.27, 0.89, -0.012], 0.004, 0.13, mats.jacketTrim, rig.bones.MaraElbowR, [Math.PI / 2, 0.0, 0.18]);
	addAttachedCapsule(proxy, "mara_rigged_shirt_fold_l", [0.055, 1.14, -0.158], 0.004, 0.32, mats.shirt, rig.bones.MaraChest, [0.0, 0.0, -0.2]);
	addAttachedCapsule(proxy, "mara_rigged_shirt_fold_r", [-0.052, 1.12, -0.158], 0.004, 0.28, mats.shirt, rig.bones.MaraChest, [0.0, 0.0, 0.16]);
	addAttachedBox(proxy, "mara_rigged_belt", [0, 0.84, -0.12], [0.42, 0.035, 0.03], mats.leather, rig.bones.MaraHips);
	addAttachedBox(proxy, "mara_rigged_belt_buckle", [0.0, 0.84, -0.157], [0.034, 0.026, 0.008], mats.metal, rig.bones.MaraHips);
	addAttachedBox(proxy, "mara_rigged_strap_buckle", [-0.075, 1.22, -0.232], [0.026, 0.034, 0.008], mats.metal, rig.bones.MaraChest, [0.0, 0.0, -0.34]);
	addAttachedBox(proxy, "mara_rigged_jacket_hem", [0, 0.86, -0.135], [0.4, 0.03, 0.024], mats.jacketTrim, rig.bones.MaraHips);
	addAttachedBox(proxy, "mara_rigged_cargo_pocket_l", [0.175, 0.52, -0.065], [0.075, 0.13, 0.028], mats.fabric, rig.bones.MaraHipL, [0, 0, -0.06]);
	addAttachedBox(proxy, "mara_rigged_cargo_pocket_r", [-0.175, 0.52, -0.065], [0.075, 0.13, 0.028], mats.fabric, rig.bones.MaraHipR, [0, 0, 0.06]);
	addAttachedCapsule(proxy, "mara_rigged_trouser_crease_l", [-0.07, 0.42, -0.065], 0.005, 0.44, mats.fabric, rig.bones.MaraHipL, [0.08, 0.0, 0.02]);
	addAttachedCapsule(proxy, "mara_rigged_trouser_crease_r", [0.07, 0.42, -0.065], 0.005, 0.44, mats.fabric, rig.bones.MaraHipR, [0.08, 0.0, -0.02]);
	addAttachedCapsule(proxy, "mara_rigged_knee_fold_l", [-0.115, 0.36, -0.042], 0.0045, 0.12, mats.fabric, rig.bones.MaraKneeL, [Math.PI / 2, 0.0, 0.02]);
	addAttachedCapsule(proxy, "mara_rigged_knee_fold_r", [0.115, 0.36, -0.042], 0.0045, 0.12, mats.fabric, rig.bones.MaraKneeR, [Math.PI / 2, 0.0, -0.02]);
	addAttachedCapsule(proxy, "mara_rigged_thigh_side_seam_l", [-0.18, 0.56, -0.01], 0.004, 0.28, mats.fabric, rig.bones.MaraHipL, [0.05, 0.0, 0.0]);
	addAttachedCapsule(proxy, "mara_rigged_thigh_side_seam_r", [0.18, 0.56, -0.01], 0.004, 0.28, mats.fabric, rig.bones.MaraHipR, [0.05, 0.0, 0.0]);
	addAttachedBox(proxy, "mara_rigged_boot_sole_l", [-0.11, 0.0, -0.19], [0.15, 0.032, 0.2], mats.leather, rig.bones.MaraAnkleL);
	addAttachedBox(proxy, "mara_rigged_boot_sole_r", [0.11, 0.0, -0.19], [0.15, 0.032, 0.2], mats.leather, rig.bones.MaraAnkleR);
	addEllipsoidDetail(proxy, "mara_rigged_boot_toe_l", [-0.11, 0.055, -0.285], [0.078, 0.036, 0.052], mats.leather, rig.bones.MaraAnkleL);
	addEllipsoidDetail(proxy, "mara_rigged_boot_toe_r", [0.11, 0.055, -0.285], [0.078, 0.036, 0.052], mats.leather, rig.bones.MaraAnkleR);
	addAttachedCapsule(proxy, "mara_rigged_boot_lace_l", [-0.11, 0.09, -0.18], 0.006, 0.11, mats.jacketTrim, rig.bones.MaraAnkleL, [Math.PI / 2, 0.0, 0.0]);
	addAttachedCapsule(proxy, "mara_rigged_boot_lace_r", [0.11, 0.09, -0.18], 0.006, 0.11, mats.jacketTrim, rig.bones.MaraAnkleR, [Math.PI / 2, 0.0, 0.0]);
	addBootRivets(proxy, "l", -0.11, rig.bones.MaraAnkleL, mats);
	addBootRivets(proxy, "r", 0.11, rig.bones.MaraAnkleR, mats);
	addHandDetails(proxy, "l", -1.0, rig.bones.MaraHandL, mats);
	addHandDetails(proxy, "r", 1.0, rig.bones.MaraHandR, mats);
	addReplacementArm(proxy, "l", 1.0, mats);
	addReplacementArm(proxy, "r", -1.0, mats);
	addStrap(proxy, "mara_rigged_cross_body_strap", rig.index, skeleton, mats.leather);
}

function createSkeleton() {
	const bones = {};
	const root = bone("MaraRoot", [0, 0, 0]);
	bones.MaraRoot = root;
	bones.MaraHips = child(root, "MaraHips", [0, 0.82, 0]);
	bones.MaraSpine = child(bones.MaraHips, "MaraSpine", [0, 0.22, 0]);
	bones.MaraChest = child(bones.MaraSpine, "MaraChest", [0, 0.3, 0.01]);
	bones.MaraNeck = child(bones.MaraChest, "MaraNeck", [0, 0.15, 0.005]);
	bones.MaraHead = child(bones.MaraNeck, "MaraHead", [0, 0.12, 0.015]);
	bones.MaraShoulderL = child(bones.MaraChest, "MaraShoulderL", [-0.2, -0.12, 0]);
	bones.MaraElbowL = child(bones.MaraShoulderL, "MaraElbowL", [-0.055, -0.24, 0.015]);
	bones.MaraHandL = child(bones.MaraElbowL, "MaraHandL", [-0.05, -0.22, 0.015]);
	bones.MaraShoulderR = child(bones.MaraChest, "MaraShoulderR", [0.2, -0.12, 0]);
	bones.MaraElbowR = child(bones.MaraShoulderR, "MaraElbowR", [0.055, -0.24, 0.015]);
	bones.MaraHandR = child(bones.MaraElbowR, "MaraHandR", [0.05, -0.22, 0.015]);
	bones.MaraHipL = child(bones.MaraHips, "MaraHipL", [-0.105, -0.1, 0]);
	bones.MaraKneeL = child(bones.MaraHipL, "MaraKneeL", [-0.01, -0.34, 0.018]);
	bones.MaraAnkleL = child(bones.MaraKneeL, "MaraAnkleL", [0.005, -0.29, -0.027]);
	bones.MaraToeL = child(bones.MaraAnkleL, "MaraToeL", [0, -0.045, -0.14]);
	bones.MaraHipR = child(bones.MaraHips, "MaraHipR", [0.105, -0.1, 0]);
	bones.MaraKneeR = child(bones.MaraHipR, "MaraKneeR", [0.01, -0.34, 0.018]);
	bones.MaraAnkleR = child(bones.MaraKneeR, "MaraAnkleR", [-0.005, -0.29, -0.027]);
	bones.MaraToeR = child(bones.MaraAnkleR, "MaraToeR", [0, -0.045, -0.14]);

	const orderedBones = [];
	root.traverse((node) => {
		if (node.isBone) {
			orderedBones.push(node);
		}
	});
	const index = Object.fromEntries(orderedBones.map((item, i) => [item.name, i]));
	return { bones, orderedBones, index };
}

function bone(name, position) {
	const result = new THREE.Bone();
	result.name = name;
	result.position.fromArray(position);
	return result;
}

function child(parent, name, position) {
	const result = bone(name, position);
	parent.add(result);
	return result;
}

function ring(center, radiusX, radiusZ, boneName) {
	return { center, radiusX, radiusZ, boneName };
}

function garmentRow(center, halfWidth, curve, boneName) {
	return { center, halfWidth, curve, boneName };
}

function skinnedTube(name, rings, boneIndex, mat, segments = 16) {
	const positions = [];
	const normals = [];
	const uvs = [];
	const skinIndices = [];
	const skinWeights = [];
	const indices = [];

	for (let r = 0; r < rings.length; r++) {
		const item = rings[r];
		for (let s = 0; s < segments; s++) {
			const angle = (s / segments) * Math.PI * 2;
			const nx = Math.cos(angle);
			const nz = Math.sin(angle);
			positions.push(
				item.center[0] + nx * item.radiusX,
				item.center[1],
				item.center[2] + nz * item.radiusZ
			);
			normals.push(nx, 0, nz);
			uvs.push(s / segments, r / Math.max(1, rings.length - 1));
			skinIndices.push(boneIndex[item.boneName], 0, 0, 0);
			skinWeights.push(1, 0, 0, 0);
		}
	}
	for (let r = 0; r < rings.length - 1; r++) {
		for (let s = 0; s < segments; s++) {
			const a = r * segments + s;
			const b = r * segments + ((s + 1) % segments);
			const c = (r + 1) * segments + s;
			const d = (r + 1) * segments + ((s + 1) % segments);
			indices.push(a, c, b, b, c, d);
		}
	}
	return skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat);
}

function skinnedGarmentPanel(name, rows, boneIndex, mat, columns = 5) {
	const positions = [];
	const normals = [];
	const uvs = [];
	const skinIndices = [];
	const skinWeights = [];
	const indices = [];

	for (let r = 0; r < rows.length; r++) {
		const item = rows[r];
		for (let c = 0; c < columns; c++) {
			const u = c / Math.max(1, columns - 1);
			const sx = (u - 0.5) * 2.0;
			const edgeRoll = Math.abs(sx) * item.curve * 0.45;
			const frontBulge = (1 - sx * sx) * item.curve;
			positions.push(
				item.center[0] + sx * item.halfWidth,
				item.center[1],
				item.center[2] - frontBulge + edgeRoll
			);
			normals.push(sx * 0.18, 0.04, -1);
			uvs.push(u, r / Math.max(1, rows.length - 1));
			skinIndices.push(boneIndex[item.boneName], 0, 0, 0);
			skinWeights.push(1, 0, 0, 0);
		}
	}
	for (let r = 0; r < rows.length - 1; r++) {
		for (let c = 0; c < columns - 1; c++) {
			const a = r * columns + c;
			const b = a + 1;
			const d = (r + 1) * columns + c + 1;
			const e = d - 1;
			indices.push(a, e, b, b, e, d);
		}
	}
	return skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat);
}

function skinnedLimb(name, start, end, radius, startBone, endBone, boneIndex, mat, segments = 14) {
	const positions = [];
	const normals = [];
	const uvs = [];
	const skinIndices = [];
	const skinWeights = [];
	const indices = [];
	const axis = new THREE.Vector3().fromArray(end).sub(new THREE.Vector3().fromArray(start));
	const up = Math.abs(axis.clone().normalize().dot(new THREE.Vector3(0, 1, 0))) > 0.92
		? new THREE.Vector3(1, 0, 0)
		: new THREE.Vector3(0, 1, 0);
	const side = new THREE.Vector3().crossVectors(axis, up).normalize();
	const forward = new THREE.Vector3().crossVectors(side, axis).normalize();
	for (let r = 0; r < 3; r++) {
		const t = r / 2;
		const center = new THREE.Vector3().fromArray(start).lerp(new THREE.Vector3().fromArray(end), t);
		const taper = 1 - Math.abs(t - 0.5) * 0.28;
		const ringRadius = Array.isArray(radius) ? radius[r] : [radius * taper, radius * taper];
		for (let s = 0; s < segments; s++) {
			const angle = (s / segments) * Math.PI * 2;
			const sx = Math.cos(angle);
			const sz = Math.sin(angle);
			const radial = side.clone().multiplyScalar(sx).add(forward.clone().multiplyScalar(sz)).normalize();
			const pos = center.clone()
				.add(side.clone().multiplyScalar(sx * ringRadius[0]))
				.add(forward.clone().multiplyScalar(sz * ringRadius[1]));
			positions.push(pos.x, pos.y, pos.z);
			normals.push(radial.x, radial.y, radial.z);
			uvs.push(s / segments, t);
			skinIndices.push(boneIndex[startBone], boneIndex[endBone], 0, 0);
			skinWeights.push(1 - t, t, 0, 0);
		}
	}
	for (let r = 0; r < 2; r++) {
		for (let s = 0; s < segments; s++) {
			const a = r * segments + s;
			const b = r * segments + ((s + 1) % segments);
			const c = (r + 1) * segments + s;
			const d = (r + 1) * segments + ((s + 1) % segments);
			indices.push(a, c, b, b, c, d);
		}
	}
	return skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat);
}

function skinnedEllipsoid(name, center, scale, boneName, boneIndex, mat, radialSegments = 18, heightSegments = 12) {
	const positions = [];
	const normals = [];
	const uvs = [];
	const skinIndices = [];
	const skinWeights = [];
	const indices = [];

	for (let y = 0; y <= heightSegments; y++) {
		const v = y / heightSegments;
		const theta = v * Math.PI;
		const sinTheta = Math.sin(theta);
		const cosTheta = Math.cos(theta);
		for (let x = 0; x < radialSegments; x++) {
			const u = x / radialSegments;
			const phi = u * Math.PI * 2;
			const localX = Math.cos(phi) * sinTheta;
			const localY = cosTheta;
			const localZ = Math.sin(phi) * sinTheta;
			const jawTaper = 1 - Math.max(0, localY * -1) * 0.18;
			const cheekLift = Math.max(0, -localZ) * Math.max(0, 1 - Math.abs(localY) * 1.4) * 0.012;
			positions.push(
				center[0] + localX * scale[0] * jawTaper,
				center[1] + localY * scale[1] + cheekLift,
				center[2] + localZ * scale[2]
			);
			normals.push(localX, localY, localZ);
			uvs.push(u, v);
			skinIndices.push(boneIndex[boneName], 0, 0, 0);
			skinWeights.push(1, 0, 0, 0);
		}
	}
	for (let y = 0; y < heightSegments; y++) {
		for (let x = 0; x < radialSegments; x++) {
			const a = y * radialSegments + x;
			const b = y * radialSegments + ((x + 1) % radialSegments);
			const c = (y + 1) * radialSegments + x;
			const d = (y + 1) * radialSegments + ((x + 1) % radialSegments);
			indices.push(a, c, b, b, c, d);
		}
	}
	return skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat);
}

function skinnedFaceMask(name, boneIndex, mat) {
	const columns = 17;
	const rows = 15;
	const positions = [];
	const normals = [];
	const uvs = [];
	const skinIndices = [];
	const skinWeights = [];
	const indices = [];
	for (let y = 0; y < rows; y++) {
		const v = y / (rows - 1);
		const yy = 1.532 + v * 0.19;
		for (let x = 0; x < columns; x++) {
			const u = x / (columns - 1);
			const sx = (u - 0.5) * 2.0;
			const absX = Math.abs(sx);
			const browCut = gaussian(v, 0.82, 0.11) * gaussian(absX, 0.45, 0.22) * 0.011;
			const eyeSocket = gaussian(v, 0.69, 0.11) * gaussian(absX, 0.43, 0.2) * 0.014;
			const noseBridge = gaussian(sx, 0.0, 0.16) * gaussian(v, 0.59, 0.28) * 0.03;
			const noseTip = gaussian(sx, 0.0, 0.22) * gaussian(v, 0.42, 0.11) * 0.018;
			const mouthPlane = gaussian(sx, 0.0, 0.44) * gaussian(v, 0.26, 0.1) * 0.014;
			const chinPlane = gaussian(sx, 0.0, 0.46) * gaussian(v, 0.08, 0.15) * 0.01;
			const cheekPlane = gaussian(absX, 0.62, 0.24) * gaussian(v, 0.47, 0.24) * 0.012;
			const nasolabial = gaussian(absX, 0.34, 0.08) * gaussian(v, 0.32, 0.2) * 0.008;
			const templeTurn = Math.pow(absX, 1.7) * 0.028;
			const jawTaper = 1.0 - Math.max(0, 0.24 - v) * 0.55 + gaussian(v, 0.14, 0.18) * 0.08;
			const browWidth = gaussian(v, 0.82, 0.16) * 0.018;
			const width = (0.108 - Math.abs(v - 0.5) * 0.02 + browWidth) * jawTaper;
			const xx = sx * width;
			const zz = -0.112
				- browCut
				- eyeSocket
				- noseBridge
				- noseTip
				- mouthPlane
				- chinPlane
				+ cheekPlane
				+ nasolabial
				+ templeTurn;
			positions.push(xx, yy, zz);
			normals.push(sx * 0.16, 0.06 - v * 0.1, -1.0);
			uvs.push(u, v);
			skinIndices.push(boneIndex.MaraHead, 0, 0, 0);
			skinWeights.push(1, 0, 0, 0);
		}
	}
	for (let y = 0; y < rows - 1; y++) {
		for (let x = 0; x < columns - 1; x++) {
			const a = y * columns + x;
			const b = a + 1;
			const c = a + columns;
			const d = c + 1;
			indices.push(a, c, b, b, c, d);
		}
	}
	return skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat);
}

function gaussian(value, center, width) {
	return Math.exp(-Math.pow((value - center) / width, 2));
}

function skinnedMesh(name, positions, normals, uvs, indices, skinIndices, skinWeights, mat) {
	const geometry = new THREE.BufferGeometry();
	geometry.setAttribute("position", new THREE.Float32BufferAttribute(positions, 3));
	geometry.setAttribute("normal", new THREE.Float32BufferAttribute(normals, 3));
	geometry.setAttribute("uv", new THREE.Float32BufferAttribute(uvs, 2));
	geometry.setAttribute("skinIndex", new THREE.Uint16BufferAttribute(skinIndices, 4));
	geometry.setAttribute("skinWeight", new THREE.Float32BufferAttribute(skinWeights, 4));
	geometry.setIndex(indices);
	geometry.computeVertexNormals();
	geometry.computeBoundingSphere();
	const mesh = new THREE.SkinnedMesh(geometry, mat);
	mesh.name = name;
	mesh.frustumCulled = false;
	return mesh;
}

function addRigidDetail(parent, name, position, radius, mat, attachBone) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 16, 10), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	attachBone.attach(mesh);
}

function addEllipsoidDetail(parent, name, position, scale, mat, attachBone) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(1, 18, 12), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.scale.fromArray(scale);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	attachBone.attach(mesh);
	return mesh;
}

function addAttachedTubeCurve(parent, name, points, radius, mat, attachBone) {
	const center = points
		.reduce((acc, point) => acc.add(new THREE.Vector3().fromArray(point)), new THREE.Vector3())
		.multiplyScalar(1 / points.length);
	const localPoints = points.map((point) => new THREE.Vector3().fromArray(point).sub(center));
	const curve = new THREE.CatmullRomCurve3(localPoints);
	const mesh = new THREE.Mesh(new THREE.TubeGeometry(curve, 18, radius, 8, false), mat);
	mesh.name = name;
	mesh.position.copy(center);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	attachBone.attach(mesh);
	return mesh;
}

function addReplacementArm(parent, suffix, side, mats) {
	addSpherePart(parent, `mara_three_replacement_shoulder_cap_${suffix}`, [0.24 * side, 1.365, 0], 0.058, mats.jacket);
	addEllipsoidPart(parent, `mara_three_replacement_shoulder_slope_${suffix}`, [0.225 * side, 1.34, -0.018], [0.075, 0.03, 0.046], mats.jacketTrim, [0.0, 0.0, 0.22 * side]);
	addCapsulePart(parent, `mara_three_replacement_upper_arm_${suffix}`, [0.255 * side, 1.22, 0], 0.047, 0.31, mats.jacket);
	addEllipsoidPart(parent, `mara_three_replacement_upper_arm_sleeve_panel_${suffix}`, [0.257 * side, 1.19, -0.037], [0.04, 0.145, 0.012], mats.jacketTrim, [0.05, 0.0, 0.1 * side]);
	addCapsulePart(parent, `mara_three_replacement_forearm_${suffix}`, [0.275 * side, 0.93, 0.01], 0.04, 0.28, mats.jacket);
	addEllipsoidPart(parent, `mara_three_replacement_forearm_cuff_${suffix}`, [0.29 * side, 0.805, 0.018], [0.047, 0.016, 0.032], mats.jacketTrim);
	addCapsulePart(parent, `mara_three_replacement_hand_${suffix}`, [0.28 * side, 0.72, 0.025], 0.04, 0.055, mats.skin);
}

function addHandDetails(parent, suffix, side, handBone, mats) {
	const x = 0.325 * side;
	addEllipsoidDetail(parent, `mara_rigged_palm_pad_${suffix}`, [x, 0.692, 0.046], [0.024, 0.018, 0.008], mats.skin, handBone);
	addAttachedCapsule(parent, `mara_rigged_thumb_${suffix}`, [x + 0.028 * side, 0.695, 0.016], 0.009, 0.055, mats.skin, handBone, [0.25, 0.0, 0.65 * side]);
	addAttachedCapsule(parent, `mara_rigged_index_finger_${suffix}`, [x + 0.012 * side, 0.66, 0.046], 0.0065, 0.065, mats.skin, handBone, [0.12, 0.0, 0.16 * side]);
	addAttachedCapsule(parent, `mara_rigged_middle_finger_${suffix}`, [x, 0.655, 0.049], 0.0068, 0.072, mats.skin, handBone, [0.1, 0.0, 0.0]);
	addAttachedCapsule(parent, `mara_rigged_ring_finger_${suffix}`, [x - 0.012 * side, 0.66, 0.046], 0.006, 0.061, mats.skin, handBone, [0.12, 0.0, -0.12 * side]);
	addEllipsoidDetail(parent, `mara_rigged_index_knuckle_${suffix}`, [x + 0.012 * side, 0.681, 0.051], [0.008, 0.004, 0.003], mats.skin, handBone);
	addEllipsoidDetail(parent, `mara_rigged_middle_knuckle_${suffix}`, [x, 0.679, 0.053], [0.0085, 0.004, 0.003], mats.skin, handBone);
	addEllipsoidDetail(parent, `mara_rigged_ring_knuckle_${suffix}`, [x - 0.012 * side, 0.681, 0.051], [0.007, 0.0036, 0.0028], mats.skin, handBone);
	addAttachedBox(parent, `mara_rigged_index_nail_${suffix}`, [x + 0.014 * side, 0.625, 0.051], [0.008, 0.004, 0.0018], mats.sclera, handBone, [0.12, 0.0, 0.12 * side]);
	addAttachedBox(parent, `mara_rigged_middle_nail_${suffix}`, [x, 0.615, 0.053], [0.008, 0.004, 0.0018], mats.sclera, handBone, [0.1, 0.0, 0.0]);
	addAttachedBox(parent, `mara_rigged_ring_nail_${suffix}`, [x - 0.013 * side, 0.626, 0.051], [0.007, 0.0038, 0.0018], mats.sclera, handBone, [0.12, 0.0, -0.1 * side]);
}

function addBootRivets(parent, suffix, x, ankleBone, mats) {
	addRigidDetail(parent, `mara_rigged_boot_rivet_outer_${suffix}`, [x + Math.sign(x) * 0.045, 0.09, -0.19], 0.008, mats.metal, ankleBone);
	addRigidDetail(parent, `mara_rigged_boot_rivet_inner_${suffix}`, [x - Math.sign(x) * 0.045, 0.09, -0.19], 0.008, mats.metal, ankleBone);
}

function addCapsulePart(parent, name, position, radius, length, mat) {
	const mesh = new THREE.Mesh(new THREE.CapsuleGeometry(radius, length, 16, 24), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	mesh.userData.mara_three_rigged_replacement_arm = true;
	parent.add(mesh);
	return mesh;
}

function addSpherePart(parent, name, position, radius, mat) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 18, 12), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	mesh.userData.mara_three_rigged_replacement_arm = true;
	parent.add(mesh);
	return mesh;
}

function addEllipsoidPart(parent, name, position, scale, mat, rotation = [0, 0, 0]) {
	const mesh = new THREE.Mesh(new THREE.SphereGeometry(1, 18, 12), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.scale.fromArray(scale);
	mesh.rotation.fromArray(rotation);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	mesh.userData.mara_three_rigged_replacement_arm = true;
	parent.add(mesh);
	return mesh;
}

function addAttachedCapsule(parent, name, position, radius, length, mat, attachBone, rotation = [0, 0, 0]) {
	const mesh = new THREE.Mesh(new THREE.CapsuleGeometry(radius, length, 12, 16), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.rotation.fromArray(rotation);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	attachBone.attach(mesh);
	return mesh;
}

function addAttachedBox(parent, name, position, size, mat, attachBone, rotation = [0, 0, 0]) {
	const mesh = new THREE.Mesh(new THREE.BoxGeometry(size[0], size[1], size[2]), mat);
	mesh.name = name;
	mesh.position.fromArray(position);
	mesh.rotation.fromArray(rotation);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	attachBone.attach(mesh);
	return mesh;
}

function addStrap(parent, name, boneIndex, skeleton, mat) {
	const mesh = skinnedTube(name, [
		ring([-0.16, 1.42, -0.15], 0.018, 0.012, "MaraChest"),
		ring([-0.06, 1.25, -0.16], 0.018, 0.012, "MaraChest"),
		ring([0.05, 1.08, -0.16], 0.018, 0.012, "MaraSpine"),
		ring([0.14, 0.9, -0.14], 0.018, 0.012, "MaraHips")
	], boneIndex, mat, 8);
	mesh.bind(skeleton);
	mesh.castShadow = true;
	mesh.receiveShadow = true;
	mesh.userData.mara_three_rigged_proxy = true;
	parent.add(mesh);
}
