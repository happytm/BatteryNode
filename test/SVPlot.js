
class SVPlot {
    constructor(id) {
        this.container = document.getElementById(id);
        this.container.innerHTML = "";
        this.canvas = document.createElement("canvas");
        this.ctx = this.canvas.getContext("2d");
        this.container.appendChild(this.canvas);
        this.graphs = {};
        this.redraw();
    }

    addGraph(label, x, y) {
        this.graphs[label] = { x, y, visible: true };
        this.redraw();
    }

    toggleGraph(label, visible) {
        if (this.graphs[label]) {
            this.graphs[label].visible = visible;
            this.redraw();
        }
    }

    redraw() {
        this.canvas.width = this.container.clientWidth - 20;
        this.canvas.height = this.container.clientHeight - 20;
        const ctx = this.ctx;
        ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        ctx.lineWidth = 2;

        const colors = ["#ff5733", "#3366cc", "#33cc33", "#cc33cc"];
        let colorIdx = 0;

        for (const label in this.graphs) {
            const graph = this.graphs[label];
            if (!graph.visible) continue;
            const x = graph.x;
            const y = graph.y;

            const maxX = Math.max(...x);
            const maxY = Math.max(...y);
            const minY = Math.min(...y);

            ctx.beginPath();
            ctx.strokeStyle = colors[colorIdx++ % colors.length];

            for (let i = 0; i < x.length; i++) {
                const px = (x[i] / maxX) * this.canvas.width;
                const py = this.canvas.height - ((y[i] - minY) / (maxY - minY)) * this.canvas.height;
                if (i === 0) {
                    ctx.moveTo(px, py);
                } else {
                    ctx.lineTo(px, py);
                }
            }
            ctx.stroke();
        }
    }
}
