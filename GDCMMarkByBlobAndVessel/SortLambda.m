function [Lambda1,Lambda2,Lambda3] = SortLambda(Lambda1,Lambda2,Lambda3)


%this function sort the Lambda

if nargin < 3
	error('parameters less than 3');
end

if length(size(Lambda1)) ~= 3 || length(size(Lambda2)) ~= 3 || length(size(Lambda3)) ~=3
	error('wrong dimensions of Lambdas');
end

for n = 1:numel(Lambda1)
	if Lambda1(n) >= Lambda2(n) && Lambda2(n) >= Lambda3(n)
		;
	else
		if Lambda1(n) < Lambda2(n)
			temp = Lambda1(n);
			Lambda1(n) = Lambda2(n);
			Lambda2(n) = temp;
		end

		if Lambda2(n) < Lambda3(n)
			temp = Lambda2(n);
			Lambda2(n) = Lambda3(n);
			Lambda3(n) = temp;
		end

		if Lambda1(n) < Lambda2(n)
			temp = Lambda1(n);
			Lambda1(n) = Lambda2(n);
			Lambda2(n) = temp;
		end
	end
end