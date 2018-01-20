format longE
p0 = 1;
p1 = 0.5714285;
p1 = vpa(p1)
epsilon = 2* 10^(-8);
e0 = 0;
e1 = epsilon*p1;

c1 = vpa(-41/14);
e_c1 = c1 * epsilon;

listoferror = double(zeros([1,40]));
listofp = double(zeros([1,40]));
listofp(1) = p0;
listofp(2) = vpa(p1);

listoferror(1) = e0;
listoferror(2) = e1;

listofans = double(zeros([1,40]));
listofans(1) = e0;
listofans(2) = e1;

constant = (14/57) * e1

curr = 2
disp(listofp)
for i = 2:40
    first = vpa((-41 * listofp(i))/14);
	second = vpa(2*(listofp(i-1)));
	temp_p = first + second;
	
	first = vpa((-41 * listoferror(i))/14);
	second = vpa(2*(listoferror(i-1)));

	temperr = first + second
	listoferror(i+1) = abs(temperr);

	ans = constant * (3.5)^i
	listofans(i+1) = abs(ans)
    listofp(i+1) = temp_p;

end

disp(listofp)
disp(listoferror)
disp(listofans)