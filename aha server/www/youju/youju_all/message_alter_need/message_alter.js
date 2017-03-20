//++++++++++++++检测昵称+++++++++++++++++++//
function checkName(){
	var name = document.getElementById('name').value;
	var warn = document.getElementById('warn1');
	
	 if(name.length > 16){
		warn.innerHTML = "昵称名字数超限！";
		warn.style.display = "block";
		return false;
	}else{
		var swear = ["艹","日你","尼玛","你妈","妈蛋","屌","叼","妈蛋","中国共产党","你爸","鸡巴","JJ","jj","操你"," ","草你"];
		for(var i = 0;i < swear.length;i ++){
			if(name.indexOf(swear[i]) != -1){
			warn.innerHTML = "您的昵称中含有敏感词或者包含空格！";
			warn.style.display = "block";
			return false;
			}
		}
	}
	return true;
}
//++++++++++++++检测长号+++++++++++++++++++//
function checkLong_phone(){
	var reg = /^1[3-8]\d{9}$/;
	var warn = document.getElementById("warn2");
	var num = document.getElementById("phone_long_number").value;
	if(num.length>0){
		if((reg.exec(num)==null) || (/^\d/.exec(num)==null)){
		warn.innerHTML = "格式错误！";
		warn.style.display = "block";
		return false;
		}
	}
	return true;
}
//++++++++++++++检测短号+++++++++++++++++++//	
function checkShort_phone(){
	var reg = /^6\d{2,6}$/;
	var warn = document.getElementById("warn3");
	var num = document.getElementById("phone_short_number").value;
	if(num.length > 0  && reg.exec(num)==null){
		warn.innerHTML = "格式错误!";
		warn.style.display = "block";
		return false;
	}
	return true;
}

//++++++++++++++检测地址+++++++++++++++++++//
function checkAddr(){
	var warn = document.getElementById("warn4");
	var addr = document.getElementById("goods_to").value;
	if(addr.value!= ''){
		var swear = ["艹","日你","尼玛","你妈","妈蛋","屌","叼","妈蛋","中国共产党","你爸","鸡巴","JJ","jj","操你"," ","草你"];
		for(var i = 0;i < swear.length;i ++){
			if(addr.indexOf(swear[i]) != -1){
			warn.innerHTML = "您的送货地址中含有敏感词或者包含空格！";
			warn.style.display = "block";
			return false;
			}
		}
	}
	return true;
}

//++++++++++++++消除警告+++++++++++++++++++//
function clearWarn(num){
	document.getElementById("warn"+num).style.display = "none";
}

document.getElementById("name").addEventListener("click",function(){clearWarn(1)},true);
document.getElementById("phone_long_number").addEventListener("click",function(){clearWarn(2)},true);
document.getElementById("phone_short_number").addEventListener("click",function(){clearWarn(3)},true);
document.getElementById("goods_to").addEventListener("click",function(){clearWarn(4)},true);

save.onclick = function(){
	var flag = 1;
	if(!checkName()) flag = 0;
	if(!checkLong_phone()) flag = 0;
	if(!checkShort_phone()) flag = 0;
	if(!checkAddr()) flag = 0;
	if(flag){
		//确认接口
	}
}

canel.onclick = function(){
	if(confirm("您的修改将不被保存，是否退出?")){
		//退出接口
	}
}
