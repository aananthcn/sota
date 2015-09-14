<html>
<title>Visteon SOTA</title>

<script src="http://code.jquery.com/jquery-latest.js"></script>

<script>
$(document).ready(function(){
	setInterval(function() {
		$("#sotatbl").load("refresh_main.php");
	}, 1000);
});
</script>


<body>

<div id="fixed">
<br>
<font size='14px' color=#ff5f00><center><b>Visteon S</b>oftware-update <b>O</b>ver <b>T</b>he <b>A</b>ir<br></center></font>
<br><br>
<form action="update_sota_id.php" method="post">
 <table width="100%" border="0" style="border:1px #cff8ff solid; border-collapse: collapse;">
  <tr>
   <td width="300" bgcolor="#cff8ff" height="45">ID: <input type="text" name="reg_id"/></td>
   <td width="480" bgcolor="#cff8ff">Update Allowed: <input type="text" name="update_allowed"/>
   <input type="submit" value="update"/></td>
   <td bgcolor="#cff8ff" align="right"><a href="releases.php">Software Releases</a></td>
  </tr>
 </table>
</form>
</div>
<br>
<div id="sotatbl">
<?php
/*************************************************************************
 * PHP CODE STARTS HERE
 */
function print_sota_table() {
	session_start();
	$username=$_SESSION['username'];
	$password=$_SESSION['password'];
	$database=$_SESSION['database'];
	$tablehdr=$_SESSION['tablehdr'];

	mysql_connect(localhost,$username,$password);
	@mysql_select_db($database) or die( "Unable to select database");


	$query="select * from sotadb.sotatbl ORDER BY id DESC";
	$sotatbl=mysql_query($query);

	$veh_rows=mysql_numrows($sotatbl);
	$veh_cols=mysql_num_fields($sotatbl);


print $tableh['name'];

	/* print vehicles - HEADER */
	echo "<table border=1 width=100%><tr>";
	$i=0;while ($i < $veh_cols) {
		$meta = mysql_fetch_field($sotatbl, $i);
		$head = $tablehdr[$meta->name];
		echo "<th bgcolor=#ffdfcf height=30>$head</th>";
		$i++;
	}
	echo "</tr>";

	/* print vehicles - DATA */
	$i=0;while ($i < $veh_rows) {
		$row=mysql_fetch_row($sotatbl);
		echo "<tr>";
		if($i & 1)
			$bgc = "#ffffff";
		else
			$bgc = "#eeeeef";

		$j=0;while ($j < $veh_cols) {
			if($j == 1) {
				echo "<td bgcolor=$bgc>" . "<font face=tahoma size=3>" . '<a href="ecu_table.php?content='. $row[$j] . '">' . $row[$j] . '</a>' . "</font>" . "</td>";
			}
			else
				echo "<td bgcolor=$bgc ><font face=tahoma size=3>$row[$j]</font></td>";

			$j++;
		}
		echo "</tr>";
		$i++;
	}
	echo "</table>";

	mysql_free_result($sotatbl);
	mysql_close();
}


print_sota_table(); 

?>
</div>

</body>
</html>
