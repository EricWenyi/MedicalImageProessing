function FBlob = CalcBlob(Lambda1, Lambda2, Lambda3)

% mex eig3volume.c
% i = double( dicomread('lung.dcm'));
% o = i(:,:,:);
% [Dxx, Dyy, Dzz, Dxy, Dxz, Dyz] = Hessian3D(o, 1);
% [Lambda1, Lambda2, Lambda3] = eig3volume(Dxx, Dyy, Dzz, Dxy, Dxz, Dyz);
% FBlob = CalcBlob(Lambda1, Lambda2, Lambda3);

if nargin < 3
	error('parameters less than 3');
end

if length(size(Lambda1)) ~= 3 || length(size(Lambda2)) ~= 3 || length(size(Lambda3)) ~=3
	error('wrong dimensions of Lambdas');
end

if isempty(Lambda1) || isempty(Lambda2) || isempty(Lambda3)
	error('some of Lambdas are empty');
end

FBlob = zeros(1, numel(Lambda1));

for n = 1:numel(Lambda1)
	if Lambda3(n) <= Lambda2(n) && Lambda2(n) <= Lambda1(n) && Lambda1(n) < 0
		if Lambda2(n) ~= 0 || Lambda3(n) ~= 0
			FBlob(n) = 4 * Lambda1(n) / Lambda3(n) / (1 + (Lambda1(n) / Lambda2(n))^2 + (Lambda2(n) / Lambda3(n))^2 + (Lambda1(n) / Lambda3(n))^2);
        end
	end
end

FBlob = reshape(FBlob, size(Lambda1, 1), size(Lambda1, 2), size(Lambda1, 3));