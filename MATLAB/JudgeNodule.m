function JudgeNodule(dcmFile)

% mex eig3volume.c

i = dicomread(dcmFile);
o = double(i(:,:,:));
[Dxx, Dyy, Dzz, Dxy, Dxz, Dyz] = Hessian3D(o);
clear o;
[Lambda1, Lambda2, Lambda3] = eig3volume(Dxx, Dyy, Dzz, Dxy, Dxz, Dyz);
clear Dxx Dyy Dzz Dxy Dxz Dyz;

FBlob = zeros(1, numel(Lambda1));

for n = 1:numel(Lambda1)
	if Lambda3(n) <= Lambda2(n) && Lambda2(n) <= Lambda1(n) && Lambda1(n) < 0
		if Lambda2(n) ~= 0 || Lambda3(n) ~= 0
			FBlob(n) = 4 * Lambda1(n) / Lambda3(n) / (1 + (Lambda1(n) / Lambda2(n))^2 + (Lambda2(n) / Lambda3(n))^2 + (Lambda1(n) / Lambda3(n))^2);
        end
	end
end

FVessel = zeros(1, numel(Lambda1));

for n = 1:numel(Lambda1)
	if Lambda3(n) <= Lambda2(n) && Lambda2(n) < 0 && Lambda2(n) < Lambda1(n)
		if Lambda2(n) ~= 0 || Lambda3(n) ~= 0
			FVessel(n) = exp(-(Lambda1(n) / Lambda2(n))^0.5) * (2 * Lambda2(n) / Lambda3(n)) / (1 + (Lambda2(n) / Lambda3(n))^2);
        end
	end
end

FVessel = abs(FVessel);
clear Lambda1 Lambda2 Lambda3;

i(find(FBlob > 0.5 & FVessel > 0 & FVessel < 0.7)) = 2047;
clear FBlob FVessel;
dicomwrite(i,'mark.dcm');