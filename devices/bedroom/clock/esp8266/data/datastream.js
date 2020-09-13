datastream = {};
t0 = undefined;
vcount = undefined;
max_size = 200;

ldr = 0;
ntc = 0;
light = 0;
cooler = 0;
clock = 0;

clock_h = 0;
clock_m = 0;
clock_s = 0;


val_r = 0;
val_g = 0;
val_b = 0;
old_val_r = 0;
old_val_g = 0;
old_val_b = 0;

Number.prototype.pad = function(size) {
    var s = String(this);
    while (s.length < (size || 2)) {s = "0" + s;}
    return s;
}

window.onload = function() {
  var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

  function map (num, in_min, in_max, out_min, out_max) {
    return (num - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  function addDataPoint(graph, time, value) {
    if (Array.isArray(value)){
      graph.data.push([time].concat(value));
    } else {
      graph.data.push([time, value]);
    }
    while (graph.data.length > max_size){
      graph.data.shift();
    }
  }

  $('#color').colorPicker({
		renderCallback: function($elm, toggled){
      var rgb = this.color.colors.RND.rgb;
      val_r = rgb.r;
      val_g = rgb.g;
      val_b = rgb.b;
		}
	});

  var myVar = setInterval(updateRGB, 100);

  function updateRGB() {
    if (old_val_r != val_r) {
      connection.send('#ctrl R ' + val_r);
      old_val_r = val_r;
    }
    if (old_val_g != val_g) {
      connection.send('#ctrl G ' + val_g);
      old_val_g = val_g;
    }
    if (old_val_b != val_b) {
      connection.send('#ctrl B ' + val_b);
      old_val_b = val_b;
    }
  }

  $('#light-btn').on('click', function(event) {
    connection.send('#ctrl L ' + (light ? 0 : 1));
  });

  $('#clock-btn').on('click', function(event) {
    clock = clock == 8 ? 0 : clock + 1;
    connection.send('#bb B M ' + clock);
  });

  function removeClassMatch(select, regex){
    $(select).removeClass (function (index, className) {
        return (className.match(regex) || []).join(' ');
    });
  }

  function androidNotify(event){
    $.ajax({
      url: 'https://maker.ifttt.com/trigger/' + event + '/with/key/bVw7WWYClYKoVUDBm51U3Z',
      type: "POST",
      // data: {"value1":"HELLO"},
      complete: function(){console.log('post: ' + event)}
    });
  }

  function updatescreen(){

    $('#clock-pb').css('width', map(clock,0,8,0,100)+'%').attr('aria-valuenow', clock);
    // $('#boiler-val').html(ntc.toFixed(1) + '°C');

    $('#ntc-val').html(ntc.toFixed(1) + '°C');

    $('#ldr-val').html(map(ldr,0,1024,0,100).toFixed(0) + '%');

    $('#clock-val').html(clock);
    // $('#time-val').html(clock_h.pad() + ':' + clock_m.pad() + ':' + clock_s.pad());

    if (light === 1){
      $('#light-btn').addClass('btn-dark').removeClass('btn-outline-dark');
    } else {
      $('#light-btn').addClass('btn-outline-dark').removeClass('btn-dark');
    }

    if (clock === 0){
      $('#clock-btn').addClass('btn-outline-dark').removeClass('btn-dark');
    } else {
      $('#clock-btn').addClass('btn-dark').removeClass('btn-outline-dark');
    }

    if (val_r === 0 && val_g === 0 && val_b === 0){
      $('#color').addClass('btn-outline-dark').removeClass('btn-dark');
    } else {
      $('#color').addClass('btn-dark').removeClass('btn-outline-dark');
    }
  }

  function saveData(label, time, value) {
    var labels = undefined;
    if (Array.isArray(label)){
      labels = label;
      label = 'all';
      //time=1067;h=18079;m=5312;s=5130;d=2630;M=23018;y=-7400;ldr=348;ntc=17.69;L=0;R=0;G=0;B=0;C=0;

      clock_h = value[1];
      clock_m = value[2];
      clock_s = value[3];

      ldr = value[6];
      ntc = value[7];
      light = value[8];
      // val_r = value[9];
      // val_g = value[10];
      // val_b = value[11];
      // cooler = value[12];

      updatescreen();
    }
    label = label.replace(/\W/g, '')
    var graph = datastream[label];
    if (!graph){
      datastream[label] = {
        'data': [],
      };
      graph = datastream[label];
    }
    addDataPoint(graph, time, value);
  }

  connection.onopen = function() {
    connection.send('Connect ' + new Date());
    connection.send('#update report 1000');
    // connection.send('!');
  };
  connection.onerror = function(error) {
    console.log('WebSocket Error ', error);
  };
  connection.onmessage = function(e) {
    if (e.data.indexOf("=") !=-1) {
      // console.log(e.data);
      var time = 0;
      var all = e.data.split(";");
      var values = [];
      var labels = [];
      if (!vcount){
        vcount = all.length;
      } else if (all.length != vcount) {
        console.log('warning invalid data received: ' + e.data);
        return;
      }
      for (i = 0; i < all.length; i++){
        label = all[i].substring(0, all[i].indexOf("="));
        labels.push(label);
        value = Number(all[i].substring(all[i].indexOf("=") + 1));
        if (i == 0){
          if (label === "time"){
            if (!t0){
              t0 = new Date();
            }
            time = new Date(t0.getTime() + value);
            if (isNaN(time)){
              console.log('warning invalid time value received: ' + e.data);
              return;
            }
          } else {
            console.log('warning invalid time label received: ' + e.data);
            return;
          }
        } else {
          values.push(value);
          saveData(label, time, value);
        }
        // console.log(label + ': ' + value);
      }
      saveData(labels, time, values);
    } else if (e.data.indexOf("Ready") !=-1) {
      console.log('Server: [', e.data, ']');
      connection.send('#update report 1000');
    } else if (e.data.indexOf("R(FF00,") !=-1) {
      s = e.data;
      code = Number(s.substring(s.indexOf(',') + 1, s.indexOf(')')));
      // switch (code) {
      //     case 0:
      // }
    } else if (e.data.indexOf("digitalWrite(") !=-1) {
      s = e.data;
      pin = Number(s.substring(s.indexOf('(') + 1, s.indexOf(',')));
      value = Number(s.substring(s.indexOf(',') + 1, s.indexOf(')')));
      if (pin === 16){
        if (value === 2){
          led = !led ? 1 : 0;
        } else {
          led = value;
        }
      }
      if (pin === 17){
        if (value === 2){
          relay = !relay ? 1 : 0;
        } else {
          relay = value;
        }
      }
      updatescreen();
    } else {
      console.log('Server: [', e.data, ']');
    }
  };
};
