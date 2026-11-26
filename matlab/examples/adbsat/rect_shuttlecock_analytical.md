# Analytic Sentman load on the rectangular shuttlecock

This note derives the exact aerodynamic force and torque on the rectangular shuttlecock (`rect_shuttlecock_faces.m`, meshes from `make_rect_shuttlecock.m`) in a pitch sweep, including the body's shadow on the wings. It serves as a code-independent reference for verifying the vat pixel calculator and ADBSat (`verify_rect_shuttlecock_analytical.m`). The implementation is `rect_shuttlecock_analytical.m`; equation numbers below are referenced in its comments.

The reference checks the *numerical* treatment (geometry, frames, shading, integration) of both codes. Both codes and this solution share the same physical model (Section 6), so the comparison is a verification, not a validation against reality.

## 1. Geometry

All coordinates are in the mesh frame (ADBSat's "geometric" frame) in metres.

| Part | Definition |
|---|---|
| Body | closed box $x \in [-a, a]$, $y, z \in [-w, w]$ with $a = 0.15$, $w = 0.05$ |
| Wings | four rectangles, length $L = 0.2$, width $b = 0.09$, zero thickness, two-sided |

Each wing is rooted on a rear edge of the body and flares backwards at 45°:

| Wing | Root edge | Direction along the wing $\hat d$ | Outer normal $n_o$ |
|---|---|---|---|
| top | $x=-a,\ z=+w,\ \lvert y\rvert \le b/2$ | $(-1, 0, 1)/\sqrt2$ | $(1, 0, 1)/\sqrt2$ |
| bottom | $x=-a,\ z=-w,\ \lvert y\rvert \le b/2$ | $(-1, 0, -1)/\sqrt2$ | $(1, 0, -1)/\sqrt2$ |
| right | $x=-a,\ y=+w,\ \lvert z\rvert \le b/2$ | $(-1, 1, 0)/\sqrt2$ | $(1, 1, 0)/\sqrt2$ |
| left | $x=-a,\ y=-w,\ \lvert z\rvert \le b/2$ | $(-1, -1, 0)/\sqrt2$ | $(1, -1, 0)/\sqrt2$ |

A wing is two coincident sheets: the *outer* face with normal $n_o$ (pointing forward and away from the body axis) and the *inner* face with $-n_o$. Since $b/2 = 0.045 < w$, the top and bottom wings lie entirely within the body's $y$-range, and the side wings lie entirely outside it ($\lvert y\rvert \ge w$).

The geometry consists of 14 flat rectangular faces: 6 body faces and 8 wing sheets. Total wetted area: $4\cdot(2a)(2w) + 2\,(2w)^2 + 8Lb = 0.14 + 0.144 = 0.284\ \mathrm{m^2}$ (body + both sheets of the four wings).

## 2. Flow and conventions

The attitude is given as ADBSat's angle of attack $\alpha$ (sideslip 0). Following ADBSat's `calc_coeff`, the gas flow direction in the mesh frame is $L_{gw}(-1, 0, 0)^T = (-\cos\alpha, 0, \sin\alpha)$. The body's velocity relative to the gas, which is vat's $v_\mathrm{rel}$, is therefore

$$\hat v = (\cos\alpha,\ 0,\ -\sin\alpha), \qquad v_\mathrm{rel} = V\,\hat v. \tag{1}$$

The gas arrives at the body along $-\hat v$. A face with outward unit normal $n$ has

$$\gamma = n\cdot\hat v = \cos\delta, \tag{2}$$

where $\delta$ is the angle between the normal and the incoming flow. The face is *windward* for $\gamma > 0$ and *leeward* for $\gamma < 0$.

Free-stream quantities come from ADBSat's `environment()`: density $\rho$, temperature $T_\infty$, mean molecular mass $\bar m$, speed $V$, and

$$c_m = \sqrt{2kT_\infty/\bar m}, \qquad s = V/c_m, \qquad q = \tfrac12\rho V^2. \tag{3}$$

The wall temperature is $T_w$ and the energy accommodation coefficient $\alpha_E$.

## 3. Sentman load on a flat face

The gas is a Maxwellian drifting at $-V\hat v$. Sentman's model integrates the incident molecular fluxes over the half-space of molecules that hit the surface and assumes fully diffuse re-emission.

**Incident number flux** per unit area, with $e = \exp(-s^2\gamma^2)$ and $E = 1 + \operatorname{erf}(s\gamma)$:

$$\dot N = \frac{\rho}{\bar m}\, \frac{c_m}{2}\left(\frac{e}{\sqrt\pi} + s\gamma E\right). \tag{4}$$

**Incident normal momentum flux.** Normalised by $q$, this is the first part of the pressure coefficient:

$$\frac{p_i}{q} = \left(\gamma^2 + \frac{1}{2s^2}\right)E + \frac{\gamma\, e}{\sqrt\pi\, s}. \tag{5a}$$

**Re-emitted normal momentum flux.** Molecules re-emitted diffusely at temperature $T_r$ leave with a mean normal momentum of $\bar m\sqrt{\pi k T_r/(2\bar m)}$ per molecule, so $p_r = \dot N \bar m \sqrt{\pi k T_r/(2\bar m)}$. With (4) and (3):

$$\frac{p_r}{q} = \frac12\,\frac{1}{s}\sqrt{\frac{T_r}{T_\infty}}\left(\sqrt\pi\,\gamma E + \frac{e}{s}\right). \tag{5b}$$

**Incident tangential momentum flux.** Each molecule carries tangential momentum $\bar m V \sin\delta$. Diffuse re-emission carries none back:

$$\frac{\tau}{q} = \sin\delta \left(\gamma E + \frac{e}{s\sqrt\pi}\right) = \sin\delta\ \tilde c. \tag{6}$$

**Reflected temperature** (hyperthermal energy accommodation). In the hyperthermal limit an incident molecule carries the kinetic energy $\tfrac12 \bar m V^2$, which corresponds to an incident temperature $T_i = \bar m V^2/(4k)$ (energy flux $2kT$ per molecule). Energy accommodation $T_r = T_i + \alpha_E (T_w - T_i)$ gives

$$\frac{1}{s}\sqrt{\frac{T_r}{T_\infty}} = \sqrt{\tfrac12\left(1 + \alpha_E\left(\frac{4kT_w}{\bar m V^2} - 1\right)\right)} = \sqrt{\tfrac12\left(1 + \alpha_E\left(\frac{2T_w}{T_\infty s^2} - 1\right)\right)}. \tag{7}$$

This is ADBSat's `coeff_sentman` and vat's Sentman with `temperature_ratio_method = 3` (Koppenwallner). The pressure coefficient is $c_p = (p_i + p_r)/q$:

$$c_p = \left(\gamma^2 + \frac{1}{2s^2}\right)E + \frac{\gamma e}{\sqrt\pi s} + \frac12\sqrt{\tfrac12\left(1 + \alpha_E\left(\frac{2T_w}{T_\infty s^2} - 1\right)\right)}\left(\sqrt\pi\gamma E + \frac{e}{s}\right). \tag{8}$$

**Force direction.** The pressure acts along $-n$. The shear acts along the tangential component of the gas velocity, $-\hat v - (-\hat v\cdot n)\,n = \gamma n - \hat v$. Its magnitude is $\sin\delta$, which cancels the $\sin\delta$ in (6). The force per unit area is therefore

$$\frac{f}{q} = -c_p\, n + \tilde c\,(\gamma n - \hat v). \tag{9}$$

This needs no division by $\sin\delta$ and holds for every $\gamma$, leeward faces ($\gamma<0$) and edge-on faces ($\gamma = 0$) included. It is identical to ADBSat's `Cf_g`, where `tauDir` $= (\gamma n - \hat v)/\sin\delta$.

**Uniform loading.** On a flat face $n$ and $\gamma$ are constant, and so is $f$. The load on the lit part of a face, with area $A_\mathrm{lit}$ and centroid $c_\mathrm{lit}$, is exactly

$$F_\mathrm{face} = f\,A_\mathrm{lit}, \qquad M_\mathrm{face} = c_\mathrm{lit} \times F_\mathrm{face}. \tag{10}$$

The total load is the sum of (10) over all 14 faces. All that remains is to determine $A_\mathrm{lit}$ and $c_\mathrm{lit}$ for each face.

## 4. Which faces are windward

For $0 < \alpha < 45^\circ$, using (1) and (2):

| Face | $n$ | $\gamma$ | Status |
|---|---|---|---|
| body front | $(1,0,0)$ | $\cos\alpha$ | windward |
| body rear | $(-1,0,0)$ | $-\cos\alpha$ | leeward |
| body $\pm y$ | $(0,\pm1,0)$ | $0$ | edge-on |
| body bottom | $(0,0,-1)$ | $\sin\alpha$ | windward |
| body top | $(0,0,1)$ | $-\sin\alpha$ | leeward |
| top wing, outer | $(1,0,1)/\sqrt2$ | $(\cos\alpha - \sin\alpha)/\sqrt2$ | windward |
| bottom wing, outer | $(1,0,-1)/\sqrt2$ | $(\cos\alpha + \sin\alpha)/\sqrt2$ | windward |
| side wings, outer | $(1,\pm1,0)/\sqrt2$ | $\cos\alpha/\sqrt2$ | windward |
| all wing inner faces | $-n_o$ | $-\gamma_o$ | leeward |

The top wing's outer face stays windward up to $\alpha = 45^\circ$, which is why the solution is restricted to $\lvert\alpha\rvert < 45^\circ$. For $\alpha < 0$ the table is mirrored in $z$: top and bottom swap roles.

## 5. Shadow of the body on the downstream wing

A windward point $P$ is lit if the ray traced upstream from it, $P + t\hat v$ with $t > 0$, does not pass through the body. All rays have constant $y$ because $\hat v_y = 0$.

**Top wing, $\alpha > 0$.** A point at distance $\ell \in [0, L]$ from the root is

$$P(\ell, y) = \left(-a - \tfrac{\ell}{\sqrt2},\ y,\ w + \tfrac{\ell}{\sqrt2}\right), \qquad \lvert y\rvert \le b/2. \tag{11}$$

The upstream ray descends with slope $\tan\alpha$. It reaches the plane $z = w$ of the body's top face after $t = \ell/(\sqrt2 \sin\alpha)$, at

$$x_\mathrm{hit} = -a + \frac{\ell}{\sqrt2}(\cot\alpha - 1).$$

For $\alpha < 45^\circ$ we have $\cot\alpha > 1$, so $x_\mathrm{hit} \ge -a$: the ray passes over the rear edge. It enters the body through the top face exactly when $x_\mathrm{hit} \le a$. Since $\lvert y\rvert \le b/2 < w$, every $y$ of the wing lies within the top face. The top face is leeward ($\gamma = -\sin\alpha$), so it is the back-facing surface that casts the shadow. The shaded part is therefore the root strip $0 \le \ell \le \ell^*$, with

$$\ell^*(\alpha) = \frac{2a\sqrt2\,\tan\alpha}{1 - \tan\alpha}. \tag{12}$$

The shadow boundary is a straight line across the full width $b$. The lit part of the top wing's outer face is

$$A_\mathrm{lit} = b\,(L - \ell^*), \qquad c_\mathrm{lit} = \text{root midpoint} + \frac{\ell^* + L}{2}\,\hat d \qquad (\ell^* < L). \tag{13}$$

The wing is fully shaded ($A_\mathrm{lit} = 0$) from

$$\alpha^* = \arctan\frac{L}{L + 2a\sqrt2} = 17.764^\circ. \tag{14}$$

At $\alpha^*$ the curves have a kink: the shadow stops growing, and the load from then on only follows $\gamma(\alpha)$ of the unshaded faces.

**Nothing else is shaded** for $0 < \alpha < 45^\circ$:
- *Bottom wing, outer face.* Its upstream rays descend from $z \le -w$ and move away from the body.
- *Side wings.* Their rays run at $\lvert y\rvert \ge w$ and never cross the body's interior ($\lvert y\rvert < w$). They also miss the top and bottom wings ($\lvert y\rvert \le b/2$).
- *Body front and bottom.* Their rays leave the body forwards and downwards, while the wings are all behind it ($x < -a$).
- *Body $\pm y$ faces.* They are edge-on ($\gamma = 0$) and have no shadow area. Their load (9) at $\gamma = 0$ is small but non-zero and is counted in full.
- *Leeward faces.* These are evaluated without shading in both codes; see Section 6.

For $\alpha < 0$ the bottom wing is shaded with $\ell^*(\lvert\alpha\rvert)$, and the force and torque are mirrored.

## 6. Conventions shared with both codes

- **Leeward faces are loaded and never shaded.** vat evaluates them on the CPU without visibility; ADBSat's `shadowAnaly` only shades forward-facing panels.
- **There are no multiple reflections** between surfaces. Every molecule interacts with one face only.
- **Shading is geometric:** a point is shaded if the straight upstream ray hits the body. Thermal spread of the incoming molecules ($s \approx 8$) is neglected in the shading, as in both codes.

These are assumptions of the panel method, so agreement with this solution says nothing about their validity.

## 7. Characteristic values

The values below use the conditions of `verify_rect_shuttlecock_analytical.m`: ADBSat's `example_GSI` environment ($h = 300$ km, F10.7 = 140, Ap = 15), which gives $\rho = 2.0068\cdot10^{-11}\ \mathrm{kg/m^3}$, $T_\infty = 943.83$ K, $\bar m = 17.69$ g/mol, $V = 7725.8$ m/s, $s = 8.2025$ and $q = 5.9892\cdot10^{-4}$ Pa, together with $\alpha_E = 1$ and $T_w = 300$ K.

Force in N and torque about the origin in N·m, both in the mesh frame. $F_y$, $M_x$ and $M_z$ vanish by symmetry; they come out at round-off, about $10^{-23}$.

| $\alpha$ | $\ell^*$ [m] | lit fraction, top wing | $F_x$ | $F_z$ | $M_y$ |
|---|---|---|---|---|---|
| 0° | 0 | 1 | $-8.1804\cdot10^{-5}$ | 0 | 0 |
| 10° | 0.09082 | 0.5459 | $-7.7415\cdot10^{-5}$ | $1.4180\cdot10^{-5}$ | $3.3631\cdot10^{-6}$ |
| 17.764° ($\alpha^*$) | 0.2 | 0 | $-7.2769\cdot10^{-5}$ | $2.4162\cdot10^{-5}$ | $5.7707\cdot10^{-6}$ |
| 30° | 0.5796 (> L) | 0 | $-7.0685\cdot10^{-5}$ | $4.1395\cdot10^{-5}$ | $7.7346\cdot10^{-6}$ |
| −10° | 0.09082 | 0.5459 (bottom wing) | $-7.7415\cdot10^{-5}$ | $-1.4180\cdot10^{-5}$ | $-3.3631\cdot10^{-6}$ |

Without shading, $\alpha = 10^\circ$ gives $F_x = -8.3239\cdot10^{-5}$ N. The body's shadow on the top wing thus removes 7.0 % of the drag at that angle.
