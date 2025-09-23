use crate::math::linalg::spectral_radius_power_iteration;

/// Compute beta0 to achieve a target R0 given contact matrix C and mean infectious duration 1/gamma.
/// R0 = spectral_radius( beta0 * C * (1/gamma) ) => beta0 = R0 * gamma / spectral_radius(C)
pub fn beta0_from_r0(contact: &[Vec<f64>], gamma: f64, r0: f64) -> f64 {
    let rho_c = spectral_radius_power_iteration(contact, 10_000, 1e-10).max(1e-12);
    r0 * gamma / rho_c
}
