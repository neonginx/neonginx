var NeoNginx = {
	UI: {
		DASHBOARD: {
			page_name:		 			document.getElementById("n_pagename"),
			total_requests: 			document.getElementById("n_dash_totalrequests"),
			active_connections: 		document.getElementById("n_dash_activeconnections"),
			servers_table: 				document.getElementById("n_dash_serverstable"),
			out_rate: 					document.getElementById("n_dash_outrate"),
			in_rate: 					document.getElementById("n_dash_inrate"),
			servers_count: 				document.getElementById("n_dash_servers")
		}
		
	},
	
	VAR: {
		TOTAL_INBOUND: 0,
		TOTAL_OUTBOUND: 0,
		LTOTAL_INBOUND: 0,
		LTOTAL_OUTBOUND: 0,
		RATE_INBOUND: 0,
		RATE_OUTBOUND: 0,
		TOTAL_SERVERS: 0
	},
	
	TOOLS: {
		FORMATNUMBER: function(num) {
		  return num.toString().replace(/(\d)(?=(\d{3})+(?!\d))/g, '$1,')
		},
		
		FORMATSIZE: function(num) {
		  if(num >= 1099511627776){
			  return (num/1099511627776).toFixed(2)+"T";
		  } else if(num >= 1073741824){
			  return (num/1073741824).toFixed(2)+"G";
		  } else if(num >= 1048576){
			  return (num/1048576).toFixed(2)+"M";
		  } else if(num >= 1024){
			  return (num/1024).toFixed(2)+"K";
		  } else {
			  return num;
		  }
		},
		
		FORMATBYTES: function(num){
			return NeoNginx.TOOLS.FORMATSIZE(num)+"B";
		},
		
		FORMATBITS: function(num){
			return NeoNginx.TOOLS.FORMATSIZE(num)+"b";
		}
	},

	FUNCTIONS: {
		
		LOGIN: function(){
			var password = document.getElementById("n_login_password").value;
			$.ajax({
			  url: '/neonginx/api/v1/login',
			  beforeSend: function(xhr) {
				xhr.setRequestHeader("NeoNginx-Password", password);
			  },
			  success: function(data) {
				if(data.STATUS == 1){
					localStorage.setItem("NEONGINX_SESSION", data.SESSION);
					window.location.href = "/";
				}
			  }
			});
			return false; // Prevent form submit
		},
		
		UPDATE: function(){
			$.ajax({
			  url: '/neonginx/api/v1/stats',
			  beforeSend: function(xhr) {
				xhr.setRequestHeader("NeoNginx-Auth", localStorage.NEONGINX_SESSION);
			  },
			  success: function(data) {
				
				if(data.STATUS == 1){
					NeoNginx.UI.DASHBOARD.total_requests.innerText = NeoNginx.TOOLS.FORMATNUMBER(data.REQUESTS_TOTAL);

					NeoNginx.UI.DASHBOARD.active_connections.innerText = NeoNginx.TOOLS.FORMATNUMBER(data.ACTIVE_CONNECTIONS);

					NeoNginx.UI.DASHBOARD.servers_table.innerHTML = null;
					NeoNginx.VAR.LTOTAL_INBOUND = NeoNginx.VAR.TOTAL_INBOUND;
					NeoNginx.VAR.LTOTAL_OUTBOUND = NeoNginx.VAR.TOTAL_OUTBOUND;
					NeoNginx.VAR.TOTAL_INBOUND = 0;
					NeoNginx.VAR.TOTAL_OUTBOUND = 0;
					NeoNginx.VAR.TOTAL_SERVERS = 0;
					for(var k in data.SERVERS){
						NeoNginx.VAR.TOTAL_OUTBOUND += data.SERVERS[k].BYTES_OUT;
						NeoNginx.VAR.TOTAL_INBOUND += data.SERVERS[k].BYTES_IN;
						NeoNginx.VAR.TOTAL_SERVERS++;
						
						NeoNginx.UI.DASHBOARD.servers_table.innerHTML += 	`<tr>
																				<td>` + data.SERVERS[k].NAME + `</td>
																				<td class="text-right">` + NeoNginx.TOOLS.FORMATNUMBER(data.SERVERS[k].REQUESTS_TOTAL) + `</td>
																				<td class="text-right">` + NeoNginx.TOOLS.FORMATBYTES(data.SERVERS[k].BYTES_IN) + `</td>
																				<td class="text-right">` + NeoNginx.TOOLS.FORMATBYTES(data.SERVERS[k].BYTES_OUT) + `</td>
																			</tr>`;
					}
					NeoNginx.VAR.RATE_INBOUND = NeoNginx.VAR.TOTAL_INBOUND - NeoNginx.VAR.LTOTAL_INBOUND;
					NeoNginx.VAR.RATE_OUTBOUND = NeoNginx.VAR.TOTAL_OUTBOUND - NeoNginx.VAR.LTOTAL_OUTBOUND;
					NeoNginx.UI.DASHBOARD.servers_count.innerText = NeoNginx.TOOLS.FORMATNUMBER(NeoNginx.VAR.TOTAL_SERVERS);
					NeoNginx.UI.DASHBOARD.out_rate.innerText = NeoNginx.TOOLS.FORMATBITS(NeoNginx.VAR.RATE_OUTBOUND*8)+"/s";
					NeoNginx.UI.DASHBOARD.in_rate.innerText = NeoNginx.TOOLS.FORMATBITS(NeoNginx.VAR.RATE_INBOUND*8)+"/s";
				}
			  },
			  error: function(data){
				  if(data.status == 401){
					  window.location.href = "/login";
				  }
			  }
			});
			
			setTimeout(NeoNginx.FUNCTIONS.UPDATE, 1000);
		},
		
		INIT: function(){
			NeoNginx.FUNCTIONS.UPDATE();
		}
	}
};



document.addEventListener('DOMContentLoaded', (event) => {
	if(document.getElementById("n_login_form") != null){
		document.getElementById("n_login_form").onsubmit = NeoNginx.FUNCTIONS.LOGIN;
	} else {
		NeoNginx.FUNCTIONS.INIT();
	}
});
