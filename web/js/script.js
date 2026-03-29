const config = {
  linear: {
    title: 'Linear Allocator',
    controls: ['allocate', 'resize_last', 'reset'],
  },
  'free-list': {
    title: 'Free List Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
  buddy: {
    title: 'Buddy Allocator',
    controls: ['allocate', 'deallocate', 'reset'],
  },
};

const pointers = new Map();
const SIZE = 1024;

const dummy_data = {
  linear: {
    name: 'Linear Allocator',
    totalBytes: 1024,
    offset: 640,
    blocks: [
      { id: 'l0', start: 0, size: 256, status: 'used', label: 'alloc #1' },
      { id: 'l1', start: 256, size: 128, status: 'used', label: 'alloc #2' },
      { id: 'l2', start: 384, size: 256, status: 'used', label: 'alloc #3' },
      { id: 'l3', start: 640, size: 384, status: 'free', label: 'free' },
    ],
    metrics: {
      used: 640,
      free: 384,
      fragmentation: 0,
      allocations: 3,
    },
  },

  'free-list': {
    name: 'Free List Allocator',
    totalBytes: 1024,
    blocks: [
      { id: 'f0', start: 0, size: 128, status: 'used', label: 'alloc #1' },
      {
        id: 'f1',
        start: 128,
        size: 256,
        status: 'free',
        label: 'free',
        nextFree: 'f3',
      },
      { id: 'f2', start: 384, size: 192, status: 'used', label: 'alloc #2' },
      {
        id: 'f3',
        start: 576,
        size: 128,
        status: 'free',
        label: 'free',
        nextFree: null,
      },
      { id: 'f4', start: 704, size: 320, status: 'used', label: 'alloc #3' },
    ],
    metrics: {
      used: 640,
      free: 384,
      fragmentation: 0.375,
      allocations: 3,
      freeListLength: 2,
    },
  },

  buddy: {
    name: 'Buddy Allocator',
    totalBytes: 1024,
    blocks: [
      {
        id: 'b0',
        start: 0,
        size: 128,
        status: 'used',
        label: 'alloc #1',
        order: 0,
      },
      {
        id: 'b1',
        start: 128,
        size: 128,
        status: 'free',
        label: 'free',
        order: 0,
      },
      {
        id: 'b2',
        start: 256,
        size: 256,
        status: 'used',
        label: 'alloc #2',
        order: 1,
      },
      {
        id: 'b3',
        start: 512,
        size: 512,
        status: 'free',
        label: 'free',
        order: 2,
      },
    ],
    metrics: {
      used: 384,
      free: 640,
      fragmentation: 0.125,
      allocations: 2,
      orders: { 0: 1, 1: 0, 2: 1 },
    },
  },
};

document.addEventListener('DOMContentLoaded', () => {
  document.querySelectorAll('.toggle').forEach((button) => {
    button.addEventListener('click', () => {
      const type = button.dataset.type;

      const previous = document.querySelector('.selected');
      if (previous) previous.classList.remove('selected');
      button.classList.add('selected');

      renderAllocator(type);
      renderDescription(type);
      renderControls(type);
    });
  });

  function renderAllocator(type) {
    const data = dummy_data[type];

    renderBlocks(type, data);
    renderMetrics(type, data.metrics);
  }

  function renderBlocks(type, state) {
    const allocator = document.getElementById('allocator');

    allocator.innerHTML = '';
    allocator.className = '';

    state.blocks.forEach((block) => {
      const element = document.createElement('div');
      element.className = `block ${block.status}`;
      element.style.left = `${(block.start / state.totalBytes) * 100}%`;
      element.style.width = `${(block.size / state.totalBytes) * 100}%`;

      const text = document.createElement('span');
      text.innerHTML = `${block.size} MB`;
      text.style.fontSize = `0.9rem`;
      element.appendChild(text);

      allocator.appendChild(element);
    });

    allocator.classList.add('view');
  }

  function renderMetrics(type, metrics) {
    const items = document.querySelectorAll('.metrics-list li span');

    items.forEach((item) => {
      const key = item.id.replace('m-', '');
      item.innerHTML = `${metrics[key]}`;
    });
  }

  function renderDescription(type) {
    const info = config[type];
    document.querySelector('.title').textContent = info.title;
  }

  function renderControls(type) {
    const info = config[type];
    const controls = document.querySelector('.controls');

    const sizes = [];
    for (let i = 16; i <= SIZE / 2; i *= 2) {
      sizes.push(i);
    }

    let html = `<h4>Controls</h4>`;

    if (info.controls.includes('allocate')) {
      const options = sizes
        .map((s) => `<option value="${s}">${s} bytes</option>`)
        .join('');
      html += `
        <div class="control-row">
            <button id="allocate-button">Allocate</button>
            <select id="allocate-size">${options}</select>
        </div>
      `;
    }
    if (info.controls.includes('deallocate')) {
      html += `
        <div class="control-row">
            <button id="deallocate-button">Deallocate</button>
            <select id="deallocate-ptr">
                <option value="" disabled selected>- select block -</option>
            </select>
        </div>
      `;
    }
    if (info.controls.includes('resize_last')) {
      html += `
        <div class="control-row">
            <button id="resize-button">Resize Last</button>
        </div>
      `;
    }
    if (info.controls.includes('reset')) {
      html += `
        <div class="control-row">
            <button id="reset-button">Reset</button>
        </div>
      `;
    }

    controls.innerHTML = html;
    attachControlHandlers(type);
  }

  function attachControlHandlers(type) {
    const allocateButton = document.getElementById('allocate-button');
    const deallocateButton = document.getElementById('deallocate-button');
    const resizeButton = document.getElementById('resize-button');
    const resetButton = document.getElementById('reset-button');

    if (allocateButton) {
      allocateButton.onclick = () => {
        const size = parseInt(
          document.getElementById('allocate-size').value,
          10,
        );
        const ptr = 'returned by module';
        if (ptr === -1) {
          console.error('allocation failed');
          return;
        }

        pointers.set(ptr, size);
        syncPointerDropdown();

        console.log(`${type}.allocate`, { ptr, size });
      };
    }

    if (deallocateButton) {
      deallocateButton.onclick = () => {
        const selected_option = document.getElementById('deallocate-ptr');
        const ptr = parseInt(selected_option.value, 10);
        if (isNaN(ptr)) {
          console.error('could not parse pointer');
          return;
        }

        console.log('call to deallocate');
        pointers.delete(ptr);
        syncPointerDropdown();

        console.log(`${type}.deallocate`, { ptr });
      };
    }

    if (resizeButton) {
      resizeButton.onclick = () => {
        console.log(`${type}.resize_last`);
      };
    }

    if (resetButton) {
      resetButton.onclick = () => {
        console.log('call to reset');
        pointers.clear();
        syncPointerDropdown();
        console.log(`${type}.reset`);
      };
    }
  }
});
