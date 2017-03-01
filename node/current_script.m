clear all;
close all;
clc
%% Make an input signal
N = 100000;
t = linspace(0,10,N);
current = cos(2*pi*50*t);
AWGN = 0.5*randn(1,N);
raw_input = current+AWGN;

% Resample signal
k=1;
n=1;
for i=1:N
    if(k==40)
       sampled_input(n)=raw_input(i);
       n=n+1; 
       k=1;
    end
    k=k+1;
end
max_val=0;

% Pick max value
for i=1:length(sampled_input)
    if max_val<sampled_input(i)
        max_val=sampled_input(i);
    end
end

var(raw_input)    
plot(t,raw_input);
