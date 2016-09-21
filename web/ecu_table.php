<html>
<title>Visteon SOTA</title>

<!--
<script src="http://code.jquery.com/jquery-latest.js"></script>

<script>
$(document).ready(function(){
	setInterval(function() {
		$("#ecu_tbl").load("refresh_main.php");
	}, 1000);
});
</script>
-->
<body>

<div id="fixed">
<br>
<font size='14px' color=#ff5f00><center><b>Visteon S</b>oftware-update <b>O</b>ver <b>T</b>he <b>A</b>ir<br></center></font>
<p align="center"> <a href="main.php">Home Page</a></p>

<form action="update_versions.php" method="post">
 <table width="100%" border="0" style="border:1px #cff8ff solid; border-collapse: collapse;">
  <tr>
   <td width="380" bgcolor="#cff8ff" height="45">ECU Name: <input type="text" name="ecu_name"/></td>
   <td width="480" bgcolor="#cff8ff">New Version: <input type="text" name="new_version"/>
   <input type="submit" value="update"/></td>
   <td bgcolor="#cff8ff" align="right"><a href="releases.php">Software Releases</a></td>
  </tr>
 </table>
</form>
</div>

<div id="ecu_tbl">
<?php
/*************************************************************************
 * PHP CODE STARTS HERE
 */

function print_sota_table() {
	session_start();
	$username=$_SESSION['username'];
	$password=$_SESSION['password'];
	$database=$_SESSION['database'];

	$link = mysqli_connect(localhost,$username,$password, $database);

	$vin = isset($_GET['content']) ? $_GET['content'] : '';
	$tbl = "ecus_"."$vin"."_tbl";
	$_SESSION['ecu_table']=$tbl;

	$query="select * from sotadb.$tbl";
	$ecu_tbl=mysqli_query($link, $query);

	$veh_rows=mysqli_num_rows($ecu_tbl);
	$veh_cols=mysqli_num_fields($ecu_tbl);

	/* print table title */
	echo "<br><font size='4px' color=#ef4f00>";
	echo "<b>Vehicle VIN: $vin</b><br>";
	echo "</font>";
	echo "<br>No of ECUs: $veh_rows<br><br>";

	/* print vehicles - HEADER */
	echo "<table border=1 cellpadding=3><tr>";
	$i=0;while ($i < $veh_cols) {
		$meta = mysqli_fetch_field($ecu_tbl);
		echo "<th bgcolor=#efefef height=40>$meta->name</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $veh_rows) {
		$row=mysqli_fetch_row($ecu_tbl);
		echo "<tr>";
		if($i & 1)
			$bgc = "#ffffff";
		else
			$bgc = "#dfffff";

		$j=0;while ($j < $veh_cols) {
			echo "<td bgcolor=$bgc ><font face=tahoma size=3>$row[$j]</font></td>";
			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysqli_free_result($ecu_tbl);
	mysqli_close();
}


print_sota_table(); 

?>
</div>

</body>
</html>
