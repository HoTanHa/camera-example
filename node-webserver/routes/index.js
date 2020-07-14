var express = require('express');
var router = express.Router();

var fs = require('fs');
var FileType = require('file-type');
const readChunk = require('read-chunk');
var moment = require('moment'); // require
const { time } = require('console');
const { static } = require('express');

/* GET home page. */
module.exports = router;

router.get('/', function (req, res) {
	res.render("index");//, { title: 'Home Page'});
});

router.get('/aaa', function (req, res) {
	res.send("welcome aaa");
	// res.render("index", { title: 'Home Page'});
});

router.get('/info', function (req, res) {
	res.render("form_info");
	// res.render("index", { title: 'Home Page'});
});


var time1_g = moment.now();
var search_file = 0;
var arrFile = [];
const objectFile = { path: "", cam: 0, listFile: ['', ''] };
var arrObjFile = [];

async function getfilename() {

	var time2 = moment.now();
	if (((time2 - time1_g) > 5 * 60 * 1000) || (search_file == 0)) {
		////truy cap 1 lan
		console.log("Search file");
		time1_g = time2;
		search_file = 1;
		arrFile = [];
		arrObjFile = [];

		var testFolder = "/home/hotanha/image/mount/";
		// let fs_item = fs.readdirSync(testFolder);	
		let fs_item;
		try {
			fs_item = fs.readdirSync(testFolder);
		} catch (error) {
			console.log("khong ton tai duong dan:" + error);
			return null;
		}


		for (let i = 0; i < fs_item.length; i++) {
			var folder_in_mount = testFolder + fs_item[i].toString();
			let stats = fs.statSync(folder_in_mount);
			if (stats.isDirectory()) {
				for (let ii = 0; ii < 4; ii++) {
					var camPath = folder_in_mount + "/cam" + ii.toString() + "/";
					var objFile = Object.create(objectFile);
					objFile.path = camPath;
					objFile.cam = ii;
					objFile.listFile = [];
					console.log(camPath);
					var fs_file_video;
					try {
						fs_file_video = fs.readdirSync(camPath);
					} catch (error) {
						console.log("faul: " + error);
					}
					for (let j = 0; j < fs_file_video.length; j++) {
						var path_video = camPath + fs_file_video[j].toString();
						let statsf = fs.statSync(path_video);
						if (statsf.isFile()) {
							let filejson = await FileType.fromFile(path_video);
							if ((filejson != null) && (filejson.ext == 'mp4')) {
								arrFile.push(fs_file_video[j].toString());
								objFile.listFile.push(fs_file_video[j].toString());
							}
						}
					}
					arrObjFile.push(objFile);
				}
			}

		}
	}
	return arrFile;
};


router.get('/file', (req, res) => {
	// returnes.render("file",{title:'QUẢN LÝ FILE VIDEO'});

	res.render("file");
})
router.get('/getfile', async function (req, res) {
	let arrFileVideo = await getfilename();
	if (!arrFileVideo || !(arrFileVideo.length) || (arrFileVideo.length == 0)) {
		res.send({ arrFile: null });
		return;
	}
	// console.log(arrFileVideo);
	var cam = req.query.cam;
	var time1 = req.query.t1;
	var time2 = req.query.t2;
	console.log(time1 + "-------------" + time2);
	var s1 = moment.unix(time1 / 1000);
	console.log(s1.format());

	let array_file = [];
	var yyyy, mm, dd, hh, min;
	var time_video;
	for (var item = 0; item < arrObjFile.length; item++) {
		if (arrObjFile[item].cam == cam) {
			for (var i = 0; i < arrObjFile[item].listFile.length; i++) {
				yyyy = arrObjFile[item].listFile[i].substring(5, 9);
				mm = arrObjFile[item].listFile[i].substring(9, 11);
				dd = arrObjFile[item].listFile[i].substring(11, 13);
				hh = arrObjFile[item].listFile[i].substring(14, 16);
				min = arrObjFile[item].listFile[i].substring(16, 18);
				time_video = moment({ y: yyyy, M: mm - 1, d: dd, h: hh, m: min, s: 0, ms: 0 });
				// console.log(time_video);
				if ((time_video >= time1) && (time_video <= time2)) {
					array_file.push(arrObjFile[item].listFile[i]);
				}
			}
		}
	}

	console.log(array_file);
	res.send({ arrFile: array_file });
	// res.send({arrFile: arrFileVideo});

});

router.get('/video', function (req, res) {
	var filename = req.query.filename;
	//const path = '/home/hotanha/image/cam0_20200625_0103.mp4'
	// const path = '/home/hotanha/image/' + filename;
	var path = '';
	var cam = parseInt(filename.substring(3, 4));
	for (var item = 0; item < arrObjFile.length; item++) {
		if (arrObjFile[item].cam === cam) {
			for (var i = 0; i < arrObjFile[item].listFile.length; i++) {
				if (filename === arrObjFile[item].listFile[i]) {
					path = arrObjFile[item].path + filename;
					console.log(path);
					break;
				}
			}
		}
	}

	const stat = fs.statSync(path)
	const fileSize = stat.size
	const range = req.headers.range
	if (range) {
		const parts = range.replace(/bytes=/, "").split("-")
		const start = parseInt(parts[0], 10)
		const end = parts[1]
			? parseInt(parts[1], 10)
			: fileSize - 1
		const chunksize = (end - start) + 1
		const file = fs.createReadStream(path, { start, end })
		const head = {
			'Content-Range': `bytes ${start}-${end}/${fileSize}`,
			'Accept-Ranges': 'bytes',
			'Content-Length': chunksize,
			'Content-Type': 'video/mp4',
		}
		res.writeHead(206, head);
		file.pipe(res);
	} else {
		const head = {
			'Content-Length': fileSize,
			'Content-Type': 'video/mp4',
		}
		res.writeHead(200, head)
		fs.createReadStream(path).pipe(res)
	}
});


function aaa() {
	// var path = "./server.js"
	// fs.stat(path, function(err, stats) {
	// 	console.log(path);
	// 	console.log();
	// 	console.log(stats);
	// 	console.log();
	// 	if (stats.isFile()) {
	// 		console.log('    file');
	// 	}
	// 	if (stats.isDirectory()) {
	// 		console.log('    directory');
	// 	}
	// 	console.log('    size: ' + stats["size"]);
	// 	console.log('    mode: ' + stats["mode"]);
	// 	console.log('    others eXecute: ' + (stats["mode"] & 1 ? 'x' : '-'));
	// 	console.log('    others Write:   ' + (stats["mode"] & 2 ? 'w' : '-'));
	// 	console.log('    others Read:    ' + (stats["mode"] & 4 ? 'r' : '-'));

	// 	console.log('    group eXecute:  ' + (stats["mode"] & 10 ? 'x' : '-'));
	// 	console.log('    group Write:    ' + (stats["mode"] & 20 ? 'w' : '-'));
	// 	console.log('    group Read:     ' + (stats["mode"] & 40 ? 'r' : '-'));

	// 	console.log('    owner eXecute:  ' + (stats["mode"] & 100 ? 'x' : '-'));
	// 	console.log('    owner Write:    ' + (stats["mode"] & 200 ? 'w' : '-'));
	// 	console.log('    owner Read:     ' + (stats["mode"] & 400 ? 'r' : '-'));


	// 	console.log('    file:           ' + (stats["mode"] & 0100000 ? 'f' : '-'));
	// 	console.log('    directory:      ' + (stats["mode"] & 0040000 ? 'd' : '-'));

	// });
	//   res.send(arrFile);
}