//++++++++++++++����ǳ�+++++++++++++++++++//
function checkName(){
	var name = document.getElementById('name').value;
	var warn = document.getElementById('warn1');
	
	 if(name.length > 16){
		warn.innerHTML = "�ǳ����������ޣ�";
		warn.style.display = "block";
		return false;
	}else{
		var swear = ["ܳ","����","����","����","�走","��","��","�走","�й�������","���","����","JJ","jj","����"," ","����"];
		for(var i = 0;i < swear.length;i ++){
			if(name.indexOf(swear[i]) != -1){
			warn.innerHTML = "�����ǳ��к������дʻ��߰����ո�";
			warn.style.display = "block";
			return false;
			}
		}
	}
	return true;
}
//++++++++++++++��ⳤ��+++++++++++++++++++//
function checkLong_phone(){
	var reg = /^1[3-8]\d{9}$/;
	var warn = document.getElementById("warn2");
	var num = document.getElementById("phone_long_number").value;
	if(num.length>0){
		if((reg.exec(num)==null) || (/^\d/.exec(num)==null)){
		warn.innerHTML = "��ʽ����";
		warn.style.display = "block";
		return false;
		}
	}
	return true;
}
//++++++++++++++���̺�+++++++++++++++++++//	
function checkShort_phone(){
	var reg = /^6\d{2,6}$/;
	var warn = document.getElementById("warn3");
	var num = document.getElementById("phone_short_number").value;
	if(num.length > 0  && reg.exec(num)==null){
		warn.innerHTML = "��ʽ����!";
		warn.style.display = "block";
		return false;
	}
	return true;
}

//++++++++++++++����ַ+++++++++++++++++++//
function checkAddr(){
	var warn = document.getElementById("warn4");
	var addr = document.getElementById("goods_to").value;
	if(addr.value!= ''){
		var swear = ["ܳ","����","����","����","�走","��","��","�走","�й�������","���","����","JJ","jj","����"," ","����"];
		for(var i = 0;i < swear.length;i ++){
			if(addr.indexOf(swear[i]) != -1){
			warn.innerHTML = "�����ͻ���ַ�к������дʻ��߰����ո�";
			warn.style.display = "block";
			return false;
			}
		}
	}
	return true;
}

//++++++++++++++��������+++++++++++++++++++//
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
		//ȷ�Ͻӿ�
	}
}

canel.onclick = function(){
	if(confirm("�����޸Ľ��������棬�Ƿ��˳�?")){
		//�˳��ӿ�
	}
}
