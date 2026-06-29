import streamlit as st
import sqlite3
import pandas as pd
import numpy as np
import plotly.graph_objects as go
import time

# --- Configuration ---
DB_PATH = "asset_tracking.db"

st.set_page_config(page_title="Marco-Polo Predictive Dashboard", layout="wide")

# --- Initialize Session State ---
if 'initialized' not in st.session_state:
    st.session_state.initialized = True
    st.session_state.hider_pos = np.array([22.0, 14.0])
    st.session_state.seeker_pos = np.array([22.0, 14.0])
    st.session_state.hider_vel = np.array([0.0, 0.0])
    st.session_state.seeker_vel = np.array([0.0, 0.0])
    st.session_state.last_hider_id = 0
    st.session_state.last_seeker_id = 0
    
    st.session_state.hider_heading = 0.0
    st.session_state.seeker_heading = 0.0

def reset_tracking(hx, hy, sx, sy):
    st.session_state.hider_pos = np.array([hx, hy])
    st.session_state.seeker_pos = np.array([sx, sy])
    st.session_state.hider_vel = np.array([0.0, 0.0])
    st.session_state.seeker_vel = np.array([0.0, 0.0])
    
    # Get current max IDs so we don't process old data after a reset
    try:
        conn = sqlite3.connect(DB_PATH)
        c = conn.cursor()
        c.execute("SELECT MAX(id) FROM hider_visualization")
        h_max = c.fetchone()[0]
        st.session_state.last_hider_id = h_max if h_max else 0
        
        c.execute("SELECT MAX(id) FROM seeker_visualization")
        s_max = c.fetchone()[0]
        st.session_state.last_seeker_id = s_max if s_max else 0
        conn.close()
    except Exception as e:
        st.warning(f"DB Error during reset: {e}")

# --- Data Fetching & Integration ---
def process_new_data(table, last_id, current_pos, current_vel, deadband, decay, scale):
    conn = sqlite3.connect(DB_PATH)
    query = f"SELECT id, acc_x, acc_y, mag_x, mag_y FROM {table} WHERE id > {last_id} ORDER BY id ASC"
    df = pd.read_sql_query(query, conn)
    conn.close()
    
    new_heading = 0.0
    if not df.empty:
        last_id = df['id'].max()
        
        for _, row in df.iterrows():
            ax, ay = row['acc_x'], row['acc_y']
            mx, my = row['mag_x'], row['mag_y']
            
            # Apply Deadband
            if abs(ax) < deadband: ax = 0
            if abs(ay) < deadband: ay = 0
            
            # Simple Euler Integration
            current_vel[0] = (current_vel[0] + ax) * decay
            current_vel[1] = (current_vel[1] + ay) * decay
            
            current_pos[0] += current_vel[0] * scale
            current_pos[1] += current_vel[1] * scale
            
            # Heading from Magnetometer (Atan2 of Y and X)
            # Note: Depending on board orientation, you may need to swap mx/my or negate them
            new_heading = np.degrees(np.arctan2(my, mx))
            
    return last_id, current_pos, current_vel, new_heading

# --- UI Layout ---
st.title("🛰️ Marco-Polo Predictive Tracking Dashboard")
st.markdown("Virtual 2D Capstone Room Tracking using IMU Dead Reckoning")

col1, col2 = st.columns([1, 4])

with col1:
    st.markdown("### Controls")
    
    with st.expander("⚙️ Physics Tuning"):
        deadband = st.slider("Accel Deadband (Gs)", 0.0, 1.0, 0.05, 0.01, help="Ignores movements smaller than this value to prevent drift from noise. Lower values make it more sensitive.")
        scale = st.slider("Scale Factor", 0.1, 10.0, 2.0, 0.1, help="Multiplies the calculated distance so tiny movements translate to feet on the grid. Higher values make the dot move further.")
        decay = st.slider("Velocity Decay", 0.0, 1.0, 0.5, 0.05, help="Simulates friction. 0.0 stops instantly when you stop moving it, 1.0 glides forever.")

    st.markdown("#### Starting Positions (Feet)")
    col_x, col_y = st.columns(2)
    with col_x:
        hx_start = st.number_input("Hider X", value=22.0, min_value=0.0, max_value=44.0, step=1.0)
        sx_start = st.number_input("Seeker X", value=22.0, min_value=0.0, max_value=44.0, step=1.0)
    with col_y:
        hy_start = st.number_input("Hider Y", value=14.0, min_value=0.0, max_value=28.0, step=1.0)
        sy_start = st.number_input("Seeker Y", value=14.0, min_value=0.0, max_value=28.0, step=1.0)

    if st.button("🔄 Apply & Reset Positions", use_container_width=True):
        reset_tracking(hx_start, hy_start, sx_start, sy_start)
        
    st.markdown("---")
    st.markdown("### Live Coordinates")
    st.metric("Hider X", f"{st.session_state.hider_pos[0]:.2f}")
    st.metric("Hider Y", f"{st.session_state.hider_pos[1]:.2f}")
    st.metric("Hider Heading", f"{st.session_state.hider_heading:.0f}°")
    
    st.markdown("---")
    st.metric("Seeker X", f"{st.session_state.seeker_pos[0]:.2f}")
    st.metric("Seeker Y", f"{st.session_state.seeker_pos[1]:.2f}")
    st.metric("Seeker Heading", f"{st.session_state.seeker_heading:.0f}°")
    
    st.markdown("---")
    st.markdown("### Live Data Stream")
    with st.expander("View Raw Database Stream", expanded=False):
        try:
            conn = sqlite3.connect(DB_PATH)
            h_df = pd.read_sql_query("SELECT id, acc_x, acc_y, mag_x, mag_y FROM hider_visualization ORDER BY id DESC LIMIT 5", conn)
            st.write("Hider Stream (Last 5)")
            st.dataframe(h_df, use_container_width=True)
            
            s_df = pd.read_sql_query("SELECT id, acc_x, acc_y, mag_x, mag_y FROM seeker_visualization ORDER BY id DESC LIMIT 5", conn)
            st.write("Seeker Stream (Last 5)")
            st.dataframe(s_df, use_container_width=True)
            conn.close()
        except Exception as e:
            st.write("No data yet.")

# Process new DB rows
try:
    st.session_state.last_hider_id, st.session_state.hider_pos, st.session_state.hider_vel, h_head = process_new_data(
        "hider_visualization", st.session_state.last_hider_id, st.session_state.hider_pos, st.session_state.hider_vel, deadband, decay, scale
    )
    if h_head != 0.0: st.session_state.hider_heading = h_head
    
    st.session_state.last_seeker_id, st.session_state.seeker_pos, st.session_state.seeker_vel, s_head = process_new_data(
        "seeker_visualization", st.session_state.last_seeker_id, st.session_state.seeker_pos, st.session_state.seeker_vel, deadband, decay, scale
    )
    if s_head != 0.0: st.session_state.seeker_heading = s_head
except Exception as e:
    st.error(f"Database error: Ensure the tables exist and are populated. {e}")

# --- Render Map ---
with col2:
    fig = go.Figure()

    # Room Boundary (44x28)
    fig.add_shape(type="rect", x0=0, y0=0, x1=44, y1=28, line=dict(color="black", width=4), fillcolor="rgba(0,0,0,0)")
    
    # Doors (Light Grey)
    fig.add_shape(type="rect", x0=14, y0=-1, x1=20, y1=0, line=dict(color="lightgrey", width=2), fillcolor="lightgrey")
    fig.add_annotation(x=17, y=-2, text="Door 2", showarrow=False)
    fig.add_shape(type="rect", x0=36, y0=-1, x1=42, y1=0, line=dict(color="lightgrey", width=2), fillcolor="lightgrey")
    fig.add_annotation(x=39, y=-2, text="Door 1", showarrow=False)
    
    # Testbeds (Dark Grey)
    fig.add_shape(type="rect", x0=1, y0=11, x1=6, y1=20, line=dict(color="darkgrey", width=2), fillcolor="rgba(169,169,169,0.4)")
    fig.add_annotation(x=3.5, y=15.5, text="Testbed J", showarrow=False)
    
    fig.add_shape(type="rect", x0=11, y0=8, x1=20, y1=20, line=dict(color="darkgrey", width=2), fillcolor="rgba(169,169,169,0.4)")
    fig.add_annotation(x=15.5, y=14, text="Testbed R", showarrow=False)
    
    # Support Beam (Grey)
    fig.add_shape(type="rect", x0=25, y0=8, x1=27, y1=10, line=dict(color="dimgrey", width=2), fillcolor="dimgrey")
    
    # Desks (Grey)
    fig.add_shape(type="rect", x0=0, y0=26, x1=44, y1=28, line=dict(color="grey", width=2), fillcolor="rgba(128,128,128,0.5)")
    fig.add_annotation(x=22, y=27, text="Top Desk", showarrow=False)
    
    fig.add_shape(type="rect", x0=43, y0=0, x1=44, y1=26, line=dict(color="grey", width=2), fillcolor="rgba(128,128,128,0.5)")
    fig.add_annotation(x=43.5, y=13, text="Right Desk", textangle=90, showarrow=False)

    # Draw Hider
    hx, hy = st.session_state.hider_pos[0], st.session_state.hider_pos[1]
    fig.add_trace(go.Scatter(
        x=[hx], y=[hy],
        mode='markers+text',
        marker=dict(size=20, color='red', symbol='circle'),
        name='Hider',
        text=['Hider'], textposition="top center"
    ))
    
    # Draw Seeker
    sx, sy = st.session_state.seeker_pos[0], st.session_state.seeker_pos[1]
    fig.add_trace(go.Scatter(
        x=[sx], y=[sy],
        mode='markers+text',
        marker=dict(size=20, color='green', symbol='square'),
        name='Seeker',
        text=['Seeker'], textposition="top center"
    ))
    
    # Calculate Arrow Endpoints for Heading (Length of arrow = 2 units)
    arrow_len = 2.0
    hx_end = hx + arrow_len * np.cos(np.radians(st.session_state.hider_heading))
    hy_end = hy + arrow_len * np.sin(np.radians(st.session_state.hider_heading))
    
    sx_end = sx + arrow_len * np.cos(np.radians(st.session_state.seeker_heading))
    sy_end = sy + arrow_len * np.sin(np.radians(st.session_state.seeker_heading))

    # Add Heading Arrows
    fig.add_annotation(x=hx_end, y=hy_end, ax=hx, ay=hy, xref='x', yref='y', axref='x', ayref='y',
                       showarrow=True, arrowhead=3, arrowsize=2, arrowwidth=2, arrowcolor='red')
    fig.add_annotation(x=sx_end, y=sy_end, ax=sx, ay=sy, xref='x', yref='y', axref='x', ayref='y',
                       showarrow=True, arrowhead=3, arrowsize=2, arrowwidth=2, arrowcolor='green')

    # Grid Settings (Capstone Room)
    fig.update_layout(
        title="Live Capstone Room Predictive Tracking",
        xaxis=dict(range=[-2, 46], title="X-Axis (Feet)", zeroline=False),
        yaxis=dict(range=[-3, 30], title="Y-Axis (Feet)", zeroline=False, scaleanchor="x", scaleratio=1),
        width=1000,
        height=700,
        showlegend=True,
    )

    st.plotly_chart(fig, use_container_width=True)

# Auto-refresh loop
time.sleep(1)
st.rerun()
