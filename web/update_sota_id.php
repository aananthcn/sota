<?php
/**************************************************************************
 * PHP CODE STARTS HERE
 */
session_start();
$username=$_SESSION['username'];
$password=$_SESSION['password'];
$database=$_SESSION['database'];
$reg_id="0";$new_version="0";$update_allowed="0";


if (isset($_POST['reg_id']))   {
	$reg_id=$_POST['reg_id'];
}
if (isset($_POST['new_version']))   {
	$new_version=$_POST['new_version'];
}
if (isset($_POST['update_allowed']))   {
	$update_allowed=$_POST['update_allowed'];
}


if(0 == strcmp($reg_id,"0")) {
	echo "Check your inputs!!";
	header('Location: ' . $_SERVER['HTTP_REFERER']);
}

mysql_connect(localhost,$username,$password);
@mysql_select_db($database) or die( "Unable to select database");

/* check if the id is valid */
$query1="SELECT vin FROM sotadb.sotatbl WHERE id='$reg_id'";
echo "$query1 <br>";
$result=mysql_query($query1) or die(mysql_error());
$num=mysql_numrows($result);
if($num != 1) {
	echo "Query failed!!";
	header('Location: ' . $_SERVER['HTTP_REFERER']);
}

/* update version */
$query2="UPDATE sotadb.sotatbl SET new_version='$new_version' WHERE id='$reg_id'";
echo "$query2 <br>";
$result=mysql_query($query2) or die(mysql_error());

/* update download condition */
$query3="UPDATE sotadb.sotatbl SET allowed='$update_allowed' WHERE id='$reg_id'";
echo "$query3 <br>";
$result=mysql_query($query3) or die(mysql_error());
echo "Success";

mysql_free_result($result);
mysql_close();
header('Location: ' . $_SERVER['HTTP_REFERER']);

?>
