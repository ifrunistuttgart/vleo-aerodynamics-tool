aero  =AeroConditions(1e-11, 934, 16*1.6605390689252e-27);
sentman =Sentman(1,0.9);
storch = Storch(1000,0.9,0.9);
maxwell = Maxwell(0.9);
schaaf = SchaafChambre(0.9,0.9);
cook = Cook(0.9);
newton = Newton();

num_steps = 500;
angles = linspace(0, pi, num_steps);

sentman_v = zeros(num_steps, 3);
storch_v = zeros(num_steps, 3);
maxwell_v = zeros(num_steps, 3);
schaaf_v = zeros(num_steps, 3);
cook_v = zeros(num_steps, 3);
newton_v = zeros(num_steps, 3);

for i = 1:num_steps
    [sentman_v(i, :),~] = sentman.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);
    [storch_v(i, :),~] = storch.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);
    [maxwell_v(i, :),~] = maxwell.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);
    [schaaf_v(i, :),~] = schaaf.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);
    [cook_v(i, :),~] = cook.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);
    [newton_v(i, :),~] = newton.calc_aero_force_torque(1.0, [cos(angles(i)) sin(angles(i)) 0], [0 0 1], [7800 0 0], 300, aero);           
end

%plot one subplot for component x and component y of the force for each model
figure;
%create a tiled layout with 2 rows and 1 column
tiledlayout(2, 1);
%plot the x component of the force for each model in the first subplot
nexttile;
plot(rad2deg(angles), sentman_v(:, 1), 'r-', 'DisplayName', 'Sentman');
hold on;
plot(rad2deg(angles), storch_v(:, 1), 'g-', 'DisplayName', 'Storch');
plot(rad2deg(angles), maxwell_v(:, 1), 'b-', 'DisplayName', 'Maxwell');
plot(rad2deg(angles), schaaf_v(:, 1), 'm-', 'DisplayName', 'Schaaf');
plot(rad2deg(angles), cook_v(:, 1), 'c-', 'DisplayName', 'Cook');
plot(rad2deg(angles), newton_v(:, 1), 'k-', 'DisplayName', 'Newton');
xlabel('Angle (deg)');
ylabel('Force (N)');
title('X Component of Aerodynamic Force');
legend('Location', 'best');

%plot the y component of the force for each model in the second subplot
nexttile;
plot(rad2deg(angles), sentman_v(:, 2), 'r-', 'DisplayName', 'Sentman');
hold on;
plot(rad2deg(angles), storch_v(:, 2), 'g-', 'DisplayName', 'Storch');
plot(rad2deg(angles), maxwell_v(:, 2), 'b-', 'DisplayName', 'Maxwell');
plot(rad2deg(angles), schaaf_v(:, 2), 'm-', 'DisplayName', 'Schaaf');
plot(rad2deg(angles), cook_v(:, 2), 'c-', 'DisplayName', 'Cook');
plot(rad2deg(angles), newton_v(:, 2),'k-', 'DisplayName', 'Newton');
xlabel('Angle (deg)');
ylabel('Force (N)');
title('Y Component of Aerodynamic Force');
legend('Location', 'best');
