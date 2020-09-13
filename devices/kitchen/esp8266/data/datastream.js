datastream = {};
t0 = undefined;
vcount = undefined;
max_size = 200;
undocked = undefined;
ldr = 0;
ntc = 0;
dhtt = 0;
dhth = 0;
led = 0;
relay = 0;
old_ldr = 0;
old_ntc = 0;
old_dhtt = 0;
old_dhth = 0;
old_led = 0;
old_relay = 0;

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

  $('#boiler-btn').on('click', function(event) {
    connection.send('#digitalWrite 17 2');
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
    $('#boiler-pb').css('width', ntc+'%').attr('aria-valuenow', ntc);
    $('#boiler-val').html(ntc.toFixed(1) + '°C');
    $('#dhtt-val').html(dhtt.toFixed(0) + '°C');
    $('#dhth-val').html(dhth.toFixed(0) + '%');

    if (isNaN(ntc)){
      undocked = setInterval(function(){ connection.send('#digitalWrite 16 2'); }, 500);
    } else {
      if (undocked) {
        clearInterval(undocked);
      }
    }

    if (relay === 1){
      // $('#boiler-pb').show();
      // $('#boiler-val').show();
      $('#boiler-btn').addClass('btn-dark').removeClass('btn-outline-dark');

      removeClassMatch('#boiler-pb', /bg-\S+/g);
      removeClassMatch('#boiler-val', /badge-\S+/g);

      if (ntc > 60){
        $('#boiler-pb').addClass('bg-danger');
        $('#boiler-val').addClass('badge-danger');
      } else if (ntc > 40) {
        $('#boiler-pb').addClass('bg-warning');
        $('#boiler-val').addClass('badge-warning');
      } else {
        $('#boiler-pb').addClass('bg-light');
        $('#boiler-val').addClass('badge-light');
      }

    } else {
      // $('#boiler-pb').hide();
      // $('#boiler-val').hide();
      removeClassMatch('#boiler-pb', /bg-\S+/g);
      removeClassMatch('#boiler-val', /badge-\S+/g);

      $('#boiler-btn').addClass('btn-outline-dark').removeClass('btn-dark');
      $('#boiler-pb').addClass('bg-dark');
      $('#boiler-val').addClass('badge-dark');
      if (old_relay === 1){
        androidNotify('boiler_90');
      }
    }

    if (ldr < 250) {
      $('#wm-btn').addClass('btn-primary').removeClass('btn-outline-dark');
      $('#wm-val').html('On');
      if (led === 0){
        connection.send('#digitalWrite 16 1');
      }
    } else {
      $('#wm-btn').addClass('btn-outline-dark').removeClass('btn-primary');
      $('#wm-val').html('Off');
      if (led === 1){
        connection.send('#digitalWrite 16 0');
        if (old_ldr < 250){
          androidNotify('ws_done');
        }
      }
    }
  }

  function saveData(label, time, value) {
    var labels = undefined;
    if (Array.isArray(label)){
      labels = label;
      label = 'all';

      old_ldr = ldr;
      old_ntc = ntc;
      old_dhtt = dhtt;
      old_dhth = dhth;
      old_led = led;
      old_relay = relay;
      ldr = value[0];
      ntc = value[1];
      dhtt = value[2];
      dhth = value[3];
      led = value[4];
      relay = value[5];

      updatescreen();
      // value[0] = map(value[0],0,1024,0,1024);
      value[1] = map(value[1],0,200,0,1024);
      value[2] = map(value[2],0,200,0,1024);
      value[3] = map(value[3],0,100,0,1024);
      value[4] = map(value[4],0,1,0,1024);
      value[5] = map(value[5],0,1,0,1024);
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
