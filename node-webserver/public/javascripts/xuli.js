$(document).ready(function () {

    $("#btnTang").hide();
    $("#btnGiam").hide();
    var sapxep = 1;
    var clickbtnShowFile = 0;
    $("#btnShowFile").click(() => {
        document.getElementById('listFile').innerHTML = "";
        var cam0_Checked = $("#radio0").is(":checked");
        var cam1_Checked = $("#radio1").is(":checked");
        var cam2_Checked = $("#radio2").is(":checked");
        var cam3_Checked = $("#radio3").is(":checked");
        // var time1 =new Date(Date.parse(document.getElementById('timeInput1').value));
        // var time2 =Date.parse(document.getElementById('timeInput2').value);
        var time1 = moment($("#timeInput1").val(), moment.DATETIME_LOCAL);//.format("YYYY-MM-DDTHH:mm");
        var time2 = moment($("#timeInput2").val(), moment.DATETIME_LOCAL);
        var camera;
        if (cam0_Checked || cam1_Checked || cam2_Checked || cam3_Checked) {
            camera = (cam0_Checked) ? 0 : ((cam1_Checked) ? 1 : ((cam2_Checked) ? 2 : 3));
            console.log("camera:" + camera);
        }
        else {
            alert("Chọn camera để xem video.");
            return;
        }
        // var time_now = new Date(new Date().getTime() - 10 * 60000);
        var time_now1 = moment().subtract(5, "minute");
        // var time_now1 = moment();
        var time_now2 = moment().subtract(2, "minute");
        console.log(time1 + "   " + time_now1);
        console.log(time_now1 - time1);
        if (time1 > time_now1) {
            alert("Chon thoi gian bat dau nho hon hien tai 5 phut");
            return;
        }
        if ((time1 >= time2)) {
            alert("Chon thoi gian ket thuc lon hon thoi gian bat dau");
            return;
        }

        if ((time2 > time_now2)) {
            time2 = time_now2;
            $('#timeInput2').val(time_now2.format("YYYY-MM-DDTHH:mm"));
        }


        if (clickbtnShowFile == 0) {
            // clickbtnShowFile = 1;
            $.get('/getfile?cam=' + camera + '&t1=' + time1 + '&t2=' + time2, (data, status) => {
                var arr = data.arrFile;
                if ((arr != null) && (arr.length != 0)) {
                    document.getElementById('listFile').innerHTML = "";
                    document.getElementById('listFile').appendChild(makeUL(arr));
                    var s1 = $('#div_video').height();
                    $('#listFile_ul').height(s1);
                    $("#btnTang").show();
                    $("#btnGiam").show();
                    sapxep = 1;
                }
                else {
                    alert('Khong co video luu trong khoang thoi gian duoc chon');
                }
            })
        }
    })


    $("#btnTang").click(() => {
        if (sapxep === 0) {
            sapxep = 1;
            var list = $('#listFile_ul');
            var listItems = list.children('li');
            list.append(listItems.get().reverse());
        }
    });
    $("#btnGiam").click(() => {
        if (sapxep === 1) {
            sapxep = 0;
            var list = $('#listFile_ul');
            var listItems = list.children('li');
            list.append(listItems.get().reverse());
        }
    });
    $(window).resize(function () {
        var s1 = $('#div_video').height();
        $('#listFile_ul').height(s1);
    });

    $('#listFile').on('click', 'a', (event) => {
        var target = getEventTarget(event);
        let name = target.id;
        var ip = location.host;
        var item = '<source src="http://' + ip.toString() + '/video?filename=' + name + '"type="video/mp4">';
        document.getElementById('video').innerHTML = item;
        const VP = document.getElementById('video'); // player
        VP.load();

    });


    //set thoi gian mac dinh cho inputtime
    // var isoStr0 = moment().add(-5, "minute").format();
    var isoStr0 = moment().subtract(5, "minute").format();
    var isoStr = moment().format();
    $('#timeInput1').val(isoStr0.substring(0, 16));
    $('#timeInput2').val(isoStr.substring(0, 16));

    // var isoStr0 = new Date(new Date().getTime() - 5 * 60000).toISOString();
    // var isoStr = new Date().toISOString();
    // $('#timeInput1').val(isoStr0.substring(0, isoStr0.length - 8));
    // $('#timeInput2').val(isoStr.substring(0, isoStr.length - 8));
    // document.getElementById("timeInput1").valueAsNumber = new Date().getTime();
    // $("#timeInput2").val(new Date().toJSON().slice(0,16));


    // const VP = document.getElementById('video') // player
    // const VPToggle = document.getElementById('toggleButton') // button
    // VPToggle.addEventListener('click', function () {
    //     if (VP.paused) VP.play()
    //     else VP.pause()
    // })
});

function getEventTarget(e) {
    e = e || window.event;
    return e.target || e.srcElement;
}

function makeUL(array) {
    // Create the list element:
    var list = document.createElement('ul');
    list.id = "listFile_ul";
    list.className = "list-group list-group-flush listFile_ul"
    for (var i = 0; i < array.length; i++) {
        var item = document.createElement('li');
        item.className = "list-group-item listFile_li ";
        item.id = "listFile_li";
        var item_span = document.createElement('a');
        item_span.className = "listFile_span";
        // item_span.type =;
        item_span.id = array[i];
        item_span.href = "#";
        item_span.appendChild(document.createTextNode(array[i]));
        item.appendChild(item_span);
        // item.appendChild(document.createTextNode(array[i]));
        list.appendChild(item);

    }

    return list;
}