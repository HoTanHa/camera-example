var express = require('express');
var router = express.Router();

var fs = require('fs');

/* GET home page. */
module.exports = router;

router.get('/', function (req, res) {
	res.render("index");//, { title: 'Home Page'});
});

router.get('/aaa', function (req, res) {
	res.send("welcome aaa");
	// res.render("index", { title: 'Home Page'});
});

